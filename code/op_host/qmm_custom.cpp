#include "register/op_def_registry.h"
#include "tiling/platform/platform_ascendc.h"
#include "tiling/matmul/bmm_tiling.h"

#include "../op_kernel/qmm_custom_tiling.h"
#include "../op_kernel/tiling_key_qmm_custom.h"

namespace optiling {

static ge::graphStatus TilingFunc(gert::TilingContext *context)
{
    auto platform = platform_ascendc::PlatformAscendC(context->GetPlatformInfo());
    uint32_t aicNum = platform.GetCoreNumAic();
    uint64_t l1Size, l0cSize, ubSize;
    platform.GetCoreMemSize(platform_ascendc::CoreMemType::L1, l1Size);
    platform.GetCoreMemSize(platform_ascendc::CoreMemType::L0_C, l0cSize);
    platform.GetCoreMemSize(platform_ascendc::CoreMemType::UB, ubSize);

    auto dtX1 = context->GetRequiredInputTensor(0)->GetDataType();
    ASCENDC_TPL_SEL_PARAM(context, static_cast<uint32_t>(dtX1));

    auto x1Shape = context->GetRequiredInputTensor(0)->GetOriginShape();
    auto x2Shape = context->GetRequiredInputTensor(1)->GetOriginShape();
    uint32_t M = static_cast<uint32_t>(x1Shape.GetDim(0));
    uint32_t K = static_cast<uint32_t>(x1Shape.GetDim(1));
    uint32_t N = static_cast<uint32_t>(x2Shape.GetDim(1));

    bool hasPertoken = (context->GetOptionalInputTensor(3) != nullptr);

    matmul_tiling::MultiCoreMatmulTiling mm;
    mm.SetAType(matmul_tiling::TPosition::GM, matmul_tiling::CubeFormat::ND,
                matmul_tiling::DataType::DT_INT8, false);
    mm.SetBType(matmul_tiling::TPosition::GM, matmul_tiling::CubeFormat::NZ,
                matmul_tiling::DataType::DT_INT8, false);
    mm.SetCType(matmul_tiling::TPosition::GM, matmul_tiling::CubeFormat::ND,
                matmul_tiling::DataType::DT_INT32);
    mm.SetBias(false);
    mm.SetDim(static_cast<int32_t>(aicNum));
    mm.SetShape(static_cast<int32_t>(M), static_cast<int32_t>(N), static_cast<int32_t>(K));
    mm.SetOrgShape(static_cast<int32_t>(M), static_cast<int32_t>(N), static_cast<int32_t>(K));
    mm.SetBufferSpace(static_cast<int32_t>(l1Size), static_cast<int32_t>(l0cSize),
                      static_cast<int32_t>(ubSize));

    optiling::TCubeTiling cubeTiling;
    if (mm.GetTiling(cubeTiling) == -1) {
        return ge::GRAPH_FAILED;
    }

    auto *tiling = context->GetTilingData<QmmCustomTilingData>();
    cubeTiling.SaveToBuffer(&tiling->cubeParams, sizeof(CubeTilingParams));

    int32_t singleCoreM = tiling->cubeParams.singleCoreM;
    int32_t singleCoreN = tiling->cubeParams.singleCoreN;

    uint32_t mBlocks = (M + static_cast<uint32_t>(singleCoreM) - 1) / static_cast<uint32_t>(singleCoreM);
    uint32_t nBlocks = (N + static_cast<uint32_t>(singleCoreN) - 1) / static_cast<uint32_t>(singleCoreN);
    uint32_t blockDim = mBlocks * nBlocks;

    tiling->M = M;
    tiling->N = N;
    tiling->K = K;
    tiling->hasPertoken = hasPertoken ? 1 : 0;
    tiling->blockDim = blockDim;
    tiling->mBlocks = mBlocks;
    tiling->nBlocks = nBlocks;
    tiling->tailM = (M % static_cast<uint32_t>(singleCoreM) == 0) ? static_cast<uint32_t>(singleCoreM)
                                                                   : M % static_cast<uint32_t>(singleCoreM);
    tiling->tailN = (N % static_cast<uint32_t>(singleCoreN) == 0) ? static_cast<uint32_t>(singleCoreN)
                                                                   : N % static_cast<uint32_t>(singleCoreN);

    context->SetBlockDim(blockDim);

    size_t *workspace = context->GetWorkspaceSizes(1);
    workspace[0] = 0;
    if (hasPertoken) {
        uint32_t perCoreWorkspace = static_cast<uint32_t>(singleCoreM) *
                                    static_cast<uint32_t>(singleCoreN) * sizeof(int32_t);
        workspace[0] = static_cast<size_t>(blockDim) * perCoreWorkspace;
    }

    return ge::GRAPH_SUCCESS;
}

}  // namespace optiling

namespace ge {

static graphStatus InferShape(gert::InferShapeContext *context)
{
    const auto *x1Shape = context->GetInputShape(0);
    const auto *x2Shape = context->GetInputShape(1);
    if (x1Shape == nullptr || x2Shape == nullptr) {
        return GRAPH_FAILED;
    }
    int64_t M = x1Shape->GetDim(0);
    int64_t N = x2Shape->GetDim(1);

    auto *outShape = context->GetOutputShape(0);
    if (outShape == nullptr) {
        return GRAPH_FAILED;
    }
    outShape->SetDim(0, M);
    outShape->SetDim(1, N);
    return GRAPH_SUCCESS;
}

static graphStatus InferDataType(gert::InferDataTypeContext *context)
{
    bool hasPertoken = (context->GetOptionalInputDataType(3) != ge::DT_UNDEFINED);
    auto outType = hasPertoken ? ge::DT_BF16 : ge::DT_INT32;
    context->SetOutputDataType(0, outType);
    return ge::GRAPH_SUCCESS;
}

}  // namespace ge

namespace ops {

class QmmCustom : public OpDef {
public:
    explicit QmmCustom(const char *name) : OpDef(name)
    {
        this->Input("x1")
            .ParamType(REQUIRED)
            .DataType({ge::DT_INT8, ge::DT_INT8})
            .Format({ge::FORMAT_ND, ge::FORMAT_ND});
        this->Input("x2")
            .ParamType(REQUIRED)
            .DataType({ge::DT_INT8, ge::DT_INT8})
            .Format({ge::FORMAT_ND, ge::FORMAT_ND});
        this->Input("scale")
            .ParamType(REQUIRED)
            .DataType({ge::DT_FLOAT, ge::DT_FLOAT})
            .Format({ge::FORMAT_ND, ge::FORMAT_ND});
        this->Input("pertoken_scale")
            .ParamType(OPTIONAL)
            .DataType({ge::DT_FLOAT, ge::DT_FLOAT})
            .Format({ge::FORMAT_ND, ge::FORMAT_ND});
        this->Output("y")
            .ParamType(REQUIRED)
            .DataType({ge::DT_BF16, ge::DT_INT32})
            .Format({ge::FORMAT_ND, ge::FORMAT_ND});
        this->SetInferShape(ge::InferShape)
            .SetInferDataType(ge::InferDataType);
        this->AICore()
            .SetTiling(optiling::TilingFunc)
            .AddConfig("ascend910b");
    }
};

OP_ADD(QmmCustom);

}  // namespace ops
