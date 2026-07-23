#include "kernel_operator.h"

#include "qmm_custom_tiling.h"
#include "tiling_key_qmm_custom.h"

using namespace matmul;

constexpr MatmulConfig MM_CFG = GetMDLConfig();

template <class DT_X1>
class KernelQmmCustom {
public:
    __aicore__ inline KernelQmmCustom() {}
    __aicore__ inline void Init(GM_ADDR x1, GM_ADDR x2, GM_ADDR scale, GM_ADDR pertokenScale,
                                GM_ADDR y, GM_ADDR workspace, GM_ADDR tiling, TPipe *pipe)
    {
        if (ASCEND_IS_AIV && !ASCEND_IS_AIC) { return; }

        auto tilingData = reinterpret_cast<__gm__ QmmCustomTilingData *>(tiling);
        M_ = tilingData[0].M;
        N_ = tilingData[0].N;
        K_ = tilingData[0].K;
        hasPertoken_ = tilingData[0].hasPertoken;
        blockDim_ = tilingData[0].blockDim;
        mBlocks_ = tilingData[0].mBlocks;
        nBlocks_ = tilingData[0].nBlocks;
        tailM_ = tilingData[0].tailM;
        tailN_ = tilingData[0].tailN;

        singleCoreM_ = static_cast<uint32_t>(tilingData[0].cubeParams.singleCoreM);
        singleCoreN_ = static_cast<uint32_t>(tilingData[0].cubeParams.singleCoreN);
        baseM_ = static_cast<uint32_t>(tilingData[0].cubeParams.baseM);
        baseN_ = static_cast<uint32_t>(tilingData[0].cubeParams.baseN);

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
        if (ASCEND_IS_AIC) {
            mm_.SetSubBlockIdx(0);
            mm_.Init(reinterpret_cast<const TCubeTiling *>(&tilingData[0].cubeParams), pipe_);
        }
    }

    __aicore__ inline void Process()
    {
        if (ASCEND_IS_AIV && !ASCEND_IS_AIC) { return; }
        if (ASCEND_IS_AIC && GetBlockIdx() >= blockDim_) { return; }

        uint32_t blockIdx = GetBlockIdx();
        uint32_t mIdx = blockIdx / nBlocks_;
        uint32_t nIdx = blockIdx % nBlocks_;

        uint32_t curM = ((mIdx == mBlocks_ - 1) && (tailM_ != 0)) ? tailM_ : singleCoreM_;
        uint32_t curN = ((nIdx == nBlocks_ - 1) && (tailN_ != 0)) ? tailN_ : singleCoreN_;

        uint32_t offsetA = mIdx * singleCoreM_ * K_;
        uint32_t alignedKBlocks = (K_ + 15) / 16;
        uint32_t nzBlockOffset = nIdx * (singleCoreN_ / 16) * alignedKBlocks * 256;
        uint32_t offsetY = mIdx * singleCoreM_ * N_ + nIdx * singleCoreN_;

        if (hasPertoken_) {
            ProcessPertoken(offsetA, nzBlockOffset, offsetY, curM, curN, blockIdx, mIdx, nIdx);
        } else {
            ProcessNonPertoken(offsetA, nzBlockOffset, offsetY, curM, curN, blockIdx);
        }
    }

    __aicore__ inline void End()
    {
        if (ASCEND_IS_AIV && !ASCEND_IS_AIC) { return; }
        mm_.End();
    }

private:
    using A_TYPE = MatmulType<TPosition::GM, CubeFormat::ND, int8_t, false>;
    using B_TYPE = MatmulType<TPosition::GM, CubeFormat::NZ, int8_t, false>;
    using C_TYPE = MatmulType<TPosition::GM, CubeFormat::ND, int32_t>;
    using BIAS_TYPE = MatmulType<TPosition::GM, CubeFormat::ND, int32_t>;

    __aicore__ inline void ProcessNonPertoken(uint32_t offsetA, uint32_t nzOffsetB, uint32_t offsetY,
                                               uint32_t curM, uint32_t curN, uint32_t blockIdx)
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

        uint64_t coreWsOffset = static_cast<uint64_t>(blockIdx) * singleCoreM_ * singleCoreN_;
        auto wsGm = workspaceGm_[coreWsOffset];

        mm_.IterateAll(wsGm, 0);
        PipeBarrier<PIPE_ALL>();
        SetAtomicNone();

        uint32_t mStart = mIdx * singleCoreM_;
        uint32_t nStart = nIdx * singleCoreN_;
        VectorDequant(workspaceGm_[coreWsOffset], offsetY, curM, curN, mStart, nStart);
    }

    __aicore__ inline void VectorDequant(GlobalTensor<int32_t> &wsGm, uint32_t offsetY,
                                          uint32_t curM, uint32_t curN, uint32_t mStart, uint32_t nStart)
    {
        constexpr uint32_t UB_F32_CAPACITY = (240U * 1024U) / sizeof(float);
        uint32_t maxChunkM = UB_F32_CAPACITY / curN;
        if (maxChunkM > 64) { maxChunkM = 64; }
        if (maxChunkM < 1) { maxChunkM = 1; }

        uint32_t chunkM = maxChunkM;

        TQue<QuePosition::VECIN, 2> inQue;
        TQue<QuePosition::VECOUT, 2> outQue;
        TBuf<> tmpBuf;

        pipe_->InitBuffer(inQue, 2, chunkM * curN * sizeof(int32_t));
        pipe_->InitBuffer(outQue, 2, chunkM * curN * sizeof(bfloat16_t));
        pipe_->InitBuffer(tmpBuf, chunkM * curN * sizeof(float) + curN * sizeof(float));

        auto tmpF32 = tmpBuf.Get<float>();
        auto scaleLocal = tmpBuf.GetWithOffset<float>(chunkM * curN, curN * sizeof(float));

        DataCopyExtParams scaleParams;
        scaleParams.blockSize = curN * sizeof(float);
        scaleParams.blockCount = 1;
        DataCopyPadExtParams<float> scalePadParams = { false, 0, 0, 0 };
        DataCopyPad(scaleLocal, scaleGm_[nStart], scaleParams, scalePadParams);
        PipeBarrier<PIPE_ALL>();

        for (uint32_t m = 0; m < curM; m += chunkM) {
            uint32_t curChunkM = (m + chunkM <= curM) ? chunkM : (curM - m);

            auto inLocal = inQue.AllocTensor<int32_t>();
            DataCopyExtParams copyInParams;
            copyInParams.blockSize = curChunkM * curN * sizeof(int32_t);
            copyInParams.blockCount = 1;
            DataCopyPadExtParams<int32_t> inPadParams = { false, 0, 0, 0 };
            DataCopyPad(inLocal, wsGm[m * curN], copyInParams, inPadParams);
            inQue.EnQue(inLocal);

            auto inData = inQue.DeQue<int32_t>();
            Cast(tmpF32, inData, RoundMode::CAST_NONE, curChunkM * curN);
            inQue.FreeTensor(inData);

            for (uint32_t r = 0; r < curChunkM; ++r) {
                Mul(tmpF32 + r * curN, tmpF32 + r * curN, scaleLocal, curN);

                uint32_t mm = mStart + m + r;
                float ptScale = pertokenScaleGm_.GetValue(mm);
                Muls(tmpF32 + r * curN, tmpF32 + r * curN, ptScale, curN);
            }

            auto outLocal = outQue.AllocTensor<bfloat16_t>();
            Cast(outLocal, tmpF32, RoundMode::CAST_ROUND, curChunkM * curN);
            outQue.EnQue(outLocal);

            auto outData = outQue.DeQue<bfloat16_t>();
            DataCopyExtParams copyOutParams;
            copyOutParams.blockSize = curChunkM * curN * sizeof(bfloat16_t);
            copyOutParams.blockCount = 1;
            DataCopyPadExtParams<bfloat16_t> outPadParams = { false, 0, 0, 0 };
            DataCopyPad(yBf16Gm_[offsetY + m * N_], outData, copyOutParams, outPadParams);
            outQue.FreeTensor(outData);
        }
    }

    GlobalTensor<int8_t> x1Gm_;
    GlobalTensor<int8_t> x2Gm_;
    GlobalTensor<float> scaleGm_;
    GlobalTensor<float> pertokenScaleGm_;
    GlobalTensor<int32_t> yInt32Gm_;
    GlobalTensor<bfloat16_t> yBf16Gm_;
    GlobalTensor<int32_t> workspaceGm_;

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
    op.Init(x1, x2, scale, pertoken_scale, y, workspace, tiling, &pipe);
    op.Process();
    op.End();
}
