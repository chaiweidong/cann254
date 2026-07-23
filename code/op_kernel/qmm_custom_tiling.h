#pragma once

#include <cstdint>

#pragma pack(push, 8)
struct CubeTilingParams {
    int32_t usedCoreNum;
    int32_t M;
    int32_t N;
    int32_t Ka;
    int32_t Kb;
    int32_t singleCoreM;
    int32_t singleCoreN;
    int32_t singleCoreK;
    int32_t baseM;
    int32_t baseN;
    int32_t baseK;
    int32_t depthA1;
    int32_t depthB1;
    int32_t stepM;
    int32_t stepN;
    int32_t isBias;
    int32_t transLength;
    int32_t iterateOrder;
    int32_t shareMode;
    int32_t shareL1Size;
    int32_t shareL0CSize;
    int32_t shareUbSize;
    int32_t batchM;
    int32_t batchN;
    int32_t singleBatchM;
    int32_t singleBatchN;
    int32_t stepKa;
    int32_t stepKb;
    int32_t depthAL1CacheUB;
    int32_t depthBL1CacheUB;
    int32_t dbL0A;
    int32_t dbL0B;
    int32_t dbL0C;
    int32_t ALayoutInfo[5];
    int32_t BLayoutInfo[5];
    int32_t CLayoutInfo[5];
    int32_t BatchNum;
    int32_t mxTypePara;
};

struct QmmCustomTilingData {
    uint32_t M;
    uint32_t N;
    uint32_t K;
    uint32_t hasPertoken;
    uint32_t blockDim;
    uint32_t mBlocks;
    uint32_t nBlocks;
    uint32_t tailM;
    uint32_t tailN;
    CubeTilingParams cubeParams;
};
#pragma pack(pop)
