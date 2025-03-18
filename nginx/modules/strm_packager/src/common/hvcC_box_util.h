#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>

#include <util/generic/buffer.h>

namespace NStrm::NPackager::NHvcCBox {
    struct TParsedHvcC {
        struct TNaluArray {
            // if ArrayCompleteness == false then stream can contain additional nal units of this type
            bool ArrayCompleteness;
            ui8 NaluType; // must be one of VPS, SPS, PPS or SEI
            TVector<TArrayRef<const ui8>> Nalus;
        };

        ui32 ConfigurationVersion;
        ui32 GeneralProfileSpace;
        ui32 GeneralTierFlag;
        ui32 GeneralProfileIdc;
        ui32 GeneralProfileCompatibilityFlags;
        ui64 GeneralConstraintIndicatorFlags;
        ui32 GeneralLevelIdc;
        ui32 MinSpatialSegmentationIdc;
        ui32 ParallelismType;
        ui32 ChromaFormat;
        ui32 BitDepthLumaMinus8;
        ui32 BitDepthChromaMinus8;
        ui32 AvgFrameRate;
        ui32 ConstantFrameRate;
        ui32 NumTemporalLayers;
        ui32 TemporalIdNested;
        ui32 LengthSizeMinusOne;

        TVector<TNaluArray> NaluArrays;
    };

    ui8 GetNaluSizeLength(const TBuffer& hvcC);

    TString DebugParse(const TBuffer& hvcC);

    TParsedHvcC Parse(const TBuffer& hvcC);
}
