#include "kernel_operator.h"
#include "adv_api/matmul_intf.h"

#include "qmm_custom_tiling.h"
#include "tiling_key_qmm_custom.h"

using namespace AscendC;
using namespace matmul;

constexpr MatmulConfig MM_CFG = GetMDLConfig();

template <class DT_X1>
class KernelQmmCustom {
public:
    __aicore__ inline KernelQmmCustom() {}
    __aicore__ inline void Init(GM_ADDR x1, GM_ADDR x2, GM_ADDR scale, GM_ADDR pertokenScale,
                                GM_ADDR y, GM_ADDR workspace, const QmmCustomTilingData *tilingData, TPipe *pipe)
    {
        M_ = tilingData->M;
        N_ = tilingData->N;
        K_ = tilingData->K;
        hasPertoken_ = tilingData->hasPertoken;
        blockDim_ = tilingData->blockDim;
        mBlocks_ = tilingData->mBlocks;
        nBlocks_ = tilingData->nBlocks;
        tailM_ = tilingData->tailM;
        tailN_ = tilingData->tailN;

        singleCoreM_ = static_cast<uint32_t>(tilingData->cubeParams.singleCoreM);
        singleCoreN_ = static_cast<uint32_t>(tilingData->cubeParams.singleCoreN);
        baseM_ = static_cast<uint32_t>(tilingData->cubeParams.baseM);
        baseN_ = static_cast<uint32_t>(tilingData->cubeParams.baseN);

        uint32_t alignedK = (K_ + 15) / 16 * 16;
        uint32_t alignedN = (N_ + 15) / 16 * 16;
        x1Gm_.SetGlobalBuffer(reinterpret_cast<__gm__ int8_t *>(x1), M_ * K_);
        x2Gm_.SetGlobalBuffer(reinterpret_cast<__gm__ int8_t *>(x2), alignedK * alignedN);
        scaleGm_.SetGlobalBuffer(reinterpret_cast<__gm__ float *>(scale), N_);

        if (hasPertoken_) {
            pertokenScaleGm_.SetGlobalBuffer(reinterpret_cast<__gm__ float *>(pertokenScale), M_);
            yBf16Gm_.SetGlobalBuffer(reinterpret_cast<__gm__ bfloat16_t *>(y), M_ * N_);
            workspaceGm_.SetGlobalBuffer(reinterpret_cast<__gm__ int32_t *>(workspace),
                                         blockDim_ * singleCoreM_ * singleCoreN_);
        } else {
            yInt32Gm_.SetGlobalBuffer(reinterpret_cast<__gm__ int32_t *>(y), M_ * N_);
        }

        pipe_ = pipe;
        if ASCEND_IS_AIC {
            mm_.SetSubBlockIdx(0);
            mm_.Init((const TCubeTiling *)&tilingData->cubeParams, pipe_);
        }
    }

    __aicore__ inline void Process()
    {
        auto blockIdx = GetBlockIdx();
        if ASCEND_IS_AIV {
            return;
        }
        if (blockIdx >= blockDim_) {
            return;
        }

        uint32_t mIdx = blockIdx / nBlocks_;
        uint32_t nIdx = blockIdx % nBlocks_;

        uint32_t curM = ((mIdx == mBlocks_ - 1) && (tailM_ != singleCoreM_)) ? tailM_ : singleCoreM_;
        uint32_t curN = ((nIdx == nBlocks_ - 1) && (tailN_ != singleCoreN_)) ? tailN_ : singleCoreN_;

        uint32_t offsetA = mIdx * singleCoreM_ * K_;
        uint32_t alignedKBlocks = (K_ + 15) / 16;
        uint32_t nzBlockOffset = nIdx * (singleCoreN_ / 16) * alignedKBlocks * 256;
        uint32_t offsetY = mIdx * singleCoreM_ * N_ + nIdx * singleCoreN_;

        if (hasPertoken_) {
            ProcessPertoken(offsetA, nzBlockOffset, offsetY, curM, curN, blockIdx, mIdx, nIdx);
        } else {
            ProcessNonPertoken(offsetA, nzBlockOffset, offsetY, curM, curN);
        }
    }

    __aicore__ inline void End()
    {
        if ASCEND_IS_AIV {
            return;
        }
        mm_.End();
    }

private:
    using A_TYPE = MatmulType<TPosition::GM, CubeFormat::ND, int8_t, false>;
    using B_TYPE = MatmulType<TPosition::GM, CubeFormat::NZ, int8_t, false>;
    using C_TYPE = MatmulType<TPosition::GM, CubeFormat::ND, int32_t>;
    using BIAS_TYPE = MatmulType<TPosition::GM, CubeFormat::ND, int32_t>;

    __aicore__ inline void ProcessNonPertoken(uint32_t offsetA, uint32_t nzOffsetB, uint32_t offsetY,
                                               uint32_t curM, uint32_t curN)
    {
        mm_.SetOrgShape(static_cast<int32_t>(curM), static_cast<int32_t>(curN), static_cast<int32_t>(K_));
        mm_.SetTensorA(x1Gm_[offsetA], false);
        mm_.SetTensorB(x2Gm_[nzOffsetB], false);
        mm_.IterateAll(yInt32Gm_[offsetY], 0);
        PipeBarrier<PIPE_ALL>();
    }

    __aicore__ inline void ProcessPertoken(uint32_t offsetA, uint32_t nzOffsetB, uint32_t offsetY,
                                            uint32_t curM, uint32_t curN, uint32_t blockIdx,
                                            uint32_t mIdx, uint32_t nIdx)
    {
        mm_.SetOrgShape(static_cast<int32_t>(curM), static_cast<int32_t>(curN), static_cast<int32_t>(K_));
        mm_.SetTensorA(x1Gm_[offsetA], false);
        mm_.SetTensorB(x2Gm_[nzOffsetB], false);

        uint64_t wsOff = static_cast<uint64_t>(blockIdx) * singleCoreM_ * singleCoreN_;
        mm_.IterateAll(workspaceGm_[wsOff], 0);
        PipeBarrier<PIPE_ALL>();
        SetAtomicNone();

        uint32_t mStart = mIdx * singleCoreM_;
        uint32_t nStart = nIdx * singleCoreN_;
        DequantVector(wsOff, offsetY, curM, curN, mStart, nStart);
    }

    __aicore__ inline void DequantVector(uint64_t wsOff, uint32_t offsetY,
                                          uint32_t curM, uint32_t curN, uint32_t mStart, uint32_t nStart)
    {
        pipe_->InitBuffer(tmpBuf_, curN * sizeof(int32_t) + curN * sizeof(float) * 2 + curN * sizeof(bfloat16_t));

        auto s32Local = tmpBuf_.Get<int32_t>(curN * sizeof(int32_t));
        auto scaleLocal = tmpBuf_.GetWithOffset<float>(curN * sizeof(int32_t), curN * sizeof(float));
        auto f32Local = tmpBuf_.GetWithOffset<float>(curN * sizeof(int32_t) + curN * sizeof(float),
                                                      curN * sizeof(float));
        auto bf16Local = tmpBuf_.GetWithOffset<bfloat16_t>(curN * sizeof(int32_t) + 2 * curN * sizeof(float),
                                                            curN * sizeof(bfloat16_t));

        DataCopyExtParams scaleCp(1, curN * sizeof(float), 0, 0, 0);
        DataCopyPadExtParams<float> scalePad;
        DataCopyPad(scaleLocal, scaleGm_[nStart], scaleCp, scalePad);

        for (uint32_t m = 0; m < curM; ++m) {
            DataCopyExtParams inCp(1, curN * sizeof(int32_t), 0, 0, 0);
            DataCopyPadExtParams<int32_t> inPad;
            DataCopyPad(s32Local, workspaceGm_[wsOff + m * curN], inCp, inPad);
            PipeBarrier<PIPE_ALL>();

            Cast(f32Local, s32Local, RoundMode::CAST_NONE, curN);

            Mul(f32Local, f32Local, scaleLocal, curN);
            PipeBarrier<PIPE_ALL>();

            float ptVal = pertokenScaleGm_.GetValue(mStart + m);
            Muls(f32Local, f32Local, ptVal, curN);
            PipeBarrier<PIPE_ALL>();

            Cast(bf16Local, f32Local, RoundMode::CAST_ROUND, curN);
            PipeBarrier<PIPE_ALL>();

            DataCopyExtParams outCp(1, curN * sizeof(bfloat16_t), 0, 0, 0);
            DataCopyPad(yBf16Gm_[offsetY + m * N_], bf16Local, outCp);
        }
    }

    GlobalTensor<int8_t> x1Gm_;
    GlobalTensor<int8_t> x2Gm_;
    GlobalTensor<float> scaleGm_;
    GlobalTensor<float> pertokenScaleGm_;
    GlobalTensor<int32_t> yInt32Gm_;
    GlobalTensor<bfloat16_t> yBf16Gm_;
    GlobalTensor<int32_t> workspaceGm_;

    TBuf<> tmpBuf_;

    MatmulImpl<A_TYPE, B_TYPE, C_TYPE, BIAS_TYPE, MM_CFG> mm_;
    TPipe *pipe_ {nullptr};

    uint32_t M_ {0};
    uint32_t N_ {0};
    uint32_t K_ {0};
    uint32_t hasPertoken_ {0};
    uint32_t blockDim_ {0};
    uint32_t mBlocks_ {0};
    uint32_t nBlocks_ {0};
    uint32_t tailM_ {0};
    uint32_t tailN_ {0};
    uint32_t singleCoreM_ {0};
    uint32_t singleCoreN_ {0};
    uint32_t baseM_ {0};
    uint32_t baseN_ {0};
};

template <typename DT_X1>
__global__ __aicore__ void qmm_custom(GM_ADDR x1, GM_ADDR x2, GM_ADDR scale, GM_ADDR pertoken_scale,
                                       GM_ADDR y, GM_ADDR workspace, GM_ADDR tiling)
{
    TPipe pipe;
    REGISTER_TILING_DEFAULT(QmmCustomTilingData);
    GET_TILING_DATA_WITH_STRUCT(QmmCustomTilingData, tiling_data, tiling);
    KernelQmmCustom<DT_X1> op;
    op.Init(x1, x2, scale, pertoken_scale, y, workspace, &tiling_data, &pipe);
    op.Process();
    op.End();
}
