#include <nginx/modules/strm_packager/src/common/hvcC_box_util.h>
#include <strm/media/transcoder/mp4muxer/io.h>

#include <util/generic/yexception.h>

namespace NStrm::NPackager::NHvcCBox {
    // hvcC box description is in ISO 14496-15 (HEVCDecoderConfigurationRecord)
    // HEVCDecoderConfigurationRecord
    // aligned(8) class HEVCDecoderConfigurationRecord {
    //     unsigned int(8) configurationVersion = 1;
    //     unsigned int(2) general_profile_space;
    //     unsigned int(1) general_tier_flag;
    //     unsigned int(5) general_profile_idc;
    //     unsigned int(32) general_profile_compatibility_flags;
    //     unsigned int(48) general_constraint_indicator_flags;
    //     unsigned int(8) general_level_idc;
    //     bit(4) reserved = ‘1111’b;
    //     unsigned int(12) min_spatial_segmentation_idc;
    //     bit(6) reserved = ‘111111’b;
    //     unsigned int(2) parallelismType;
    //     bit(6) reserved = ‘111111’b;
    //     unsigned int(2) chromaFormat;
    //     bit(5) reserved = ‘11111’b;
    //     unsigned int(3) bitDepthLumaMinus8;
    //     bit(5) reserved = ‘11111’b;
    //     unsigned int(3) bitDepthChromaMinus8;
    //     bit(16) avgFrameRate;
    //     bit(2) constantFrameRate;
    //     bit(3) numTemporalLayers;
    //     bit(1) temporalIdNested;
    //     unsigned int(2) lengthSizeMinusOne;
    //     unsigned int(8) numOfArrays;
    //     for (j=0; j < numOfArrays; j++) {
    //         bit(1) array_completeness;
    //         unsigned int(1) reserved = 0;
    //         unsigned int(6) NAL_unit_type;
    //         unsigned int(16) numNalus;
    //         for (i=0; i< numNalus; i++) {
    //             unsigned int(16) nalUnitLength;
    //             bit(8*nalUnitLength) nalUnit;
    //         }
    //     }
    // }

    TString DebugParse(const TBuffer& hvcC) {
        const TParsedHvcC parsed = Parse(hvcC);

        TStringBuilder hvccStr;
        hvccStr
            << "\n  configurationVersion                : " << parsed.ConfigurationVersion
            << "\n  general_profile_space               : " << parsed.GeneralProfileSpace
            << "\n  general_tier_flag                   : " << parsed.GeneralTierFlag
            << "\n  general_profile_idc                 : " << parsed.GeneralProfileIdc
            << "\n  general_profile_compatibility_flags : " << parsed.GeneralProfileCompatibilityFlags
            << "\n  general_constraint_indicator_flags  : " << parsed.GeneralConstraintIndicatorFlags
            << "\n  general_level_idc                   : " << parsed.GeneralLevelIdc
            << "\n  min_spatial_segmentation_idc        : " << parsed.MinSpatialSegmentationIdc
            << "\n  parallelismType                     : " << parsed.ParallelismType
            << "\n  chromaFormat                        : " << parsed.ChromaFormat
            << "\n  bitDepthLumaMinus8                  : " << parsed.BitDepthLumaMinus8
            << "\n  bitDepthChromaMinus8                : " << parsed.BitDepthChromaMinus8
            << "\n  avgFrameRate                        : " << parsed.AvgFrameRate
            << "\n  constantFrameRate                   : " << parsed.ConstantFrameRate
            << "\n  numTemporalLayers                   : " << parsed.NumTemporalLayers
            << "\n  temporalIdNested                    : " << parsed.TemporalIdNested
            << "\n  lengthSizeMinusOne                  : " << parsed.LengthSizeMinusOne
            << "\n  numOfArrays                         : " << parsed.NaluArrays.size();

        for (const auto& arr : parsed.NaluArrays) {
            hvccStr
                << "\n    array_completeness                  : " << arr.ArrayCompleteness
                << "\n    NAL_unit_type                       : " << arr.NaluType
                << "\n    numNalus                            : " << arr.Nalus.size();

            for (const auto& nalu : arr.Nalus) {
                hvccStr
                    << "\n      nalUnitLength                       : " << nalu.size();
            }
        }

        return hvccStr;
    }

    TParsedHvcC Parse(const TBuffer& hvcC) {
        using NMP4Muxer::ReadUi16BigEndian;
        using NMP4Muxer::ReadUi32BigEndian;
        using NMP4Muxer::ReadUI8;

        TParsedHvcC res;

        NStrm::NMP4Muxer::TBlobReader reader((ui8 const*)hvcC.Data(), hvcC.Size());

        ui8 b8, b16;

        res.ConfigurationVersion = ReadUI8(reader);

        // compatible changes will not change the version
        Y_ENSURE(res.ConfigurationVersion == 1);

        b8 = ReadUI8(reader);

        res.GeneralProfileSpace = (b8 >> 6) & 0b11;
        res.GeneralTierFlag = (b8 >> 5) & 0b1;
        res.GeneralProfileIdc = (b8 >> 0) & 0b11111;

        res.GeneralProfileCompatibilityFlags = ReadUi32BigEndian(reader);

        b16 = ReadUi16BigEndian(reader);
        res.GeneralConstraintIndicatorFlags = (ui64(b16) << 32) | ReadUi32BigEndian(reader);

        res.GeneralLevelIdc = ReadUI8(reader);
        res.MinSpatialSegmentationIdc = ReadUi16BigEndian(reader) & 0b0000111111111111;
        res.ParallelismType = ReadUI8(reader) & 0b00000011;
        res.ChromaFormat = ReadUI8(reader) & 0b00000011;
        res.BitDepthLumaMinus8 = ReadUI8(reader) & 0b00000111;
        res.BitDepthChromaMinus8 = ReadUI8(reader) & 0b00000111;

        res.AvgFrameRate = ReadUi16BigEndian(reader);

        b8 = ReadUI8(reader);
        res.ConstantFrameRate = (b8 & 0b11000000) >> 6;
        res.NumTemporalLayers = (b8 & 0b00111000) >> 3;
        res.TemporalIdNested = (b8 & 0b00000100) >> 2;
        res.LengthSizeMinusOne = (b8 & 0b00000011) >> 0;

        const int numOfArrays = ReadUI8(reader);
        res.NaluArrays.resize(numOfArrays);

        for (int j = 0; j < numOfArrays; ++j) {
            TParsedHvcC::TNaluArray& arr = res.NaluArrays[j];

            b8 = ReadUI8(reader);
            arr.ArrayCompleteness = (b8 & 0b10000000) >> 7;
            arr.NaluType = b8 & 0b00111111;

            const int numNalus = ReadUi16BigEndian(reader);

            for (int i = 0; i < numNalus; ++i) {
                const ui16 nalUnitLength = ReadUi16BigEndian(reader);

                arr.Nalus.emplace_back((ui8 const*)hvcC.data() + reader.GetPosition(), nalUnitLength);

                reader.SetPosition(reader.GetPosition() + nalUnitLength);
            }
        }

        Y_ENSURE(reader.GetPosition() == hvcC.Size());

        return res;
    }

    ui8 GetNaluSizeLength(const TBuffer& hvcC) {
        using NMP4Muxer::ReadUI8;

        constexpr int nalSizeBytePos = 21;

        Y_ENSURE(hvcC.Size() > nalSizeBytePos);

        NStrm::NMP4Muxer::TBlobReader reader((ui8 const*)hvcC.Data(), hvcC.Size());

        const ui8 configurationVersion = ReadUI8(reader);

        // compatible changes will not change the version
        Y_ENSURE(configurationVersion == 1);

        reader.SetPosition(nalSizeBytePos);

        const ui8 b8 = ReadUI8(reader);
        const ui8 lengthSizeMinusOne = (b8 & 0b00000011) >> 0;

        return lengthSizeMinusOne + 1;
    }

}
