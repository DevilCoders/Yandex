#pragma once

#include <strm/media/transcoder/mp4muxer/bit_reader.h>

#include <util/generic/array_ref.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>

namespace NStrm::NPackager::Nh2645 {
    // from talbe 'NAL unit type codes' of ISO 14496-10
    enum class EAvcNalType: ui32 {
        UNSPECIFIED = 0,      //  Unspecified
        SLICE = 1,            //  Coded slice of a non-IDR picture slice_layer_without_partitioning_rbsp( )
        DPA = 2,              //  Coded slice data partition A slice_data_partition_a_layer_rbsp( )
        DPB = 3,              //  Coded slice data partition B slice_data_partition_b_layer_rbsp( )
        DPC = 4,              //  Coded slice data partition C slice_data_partition_c_layer_rbsp( )
        IDR_SLICE = 5,        //  Coded slice of an IDR picture slice_layer_without_partitioning_rbsp( )
        SEI = 6,              //  Supplemental enhancement information (SEI) sei_rbsp( )
        SPS = 7,              //  Sequence parameter set seq_parameter_set_rbsp( )
        PPS = 8,              //  Picture parameter set pic_parameter_set_rbsp( )
        AUD = 9,              //  Access unit delimiter access_unit_delimiter_rbsp( )
        END_SEQUENCE = 10,    //  End of sequence end_of_seq_rbsp( )
        END_STREAM = 11,      //  End of stream end_of_stream_rbsp( )
        FILLER_DATA = 12,     //  Filler data filler_data_rbsp( )
        SPS_EXT = 13,         // TODO: description comment
        AUXILIARY_SLICE = 19, // TODO: description comment
    };

    enum class EHevcNalType: ui32 {
        TRAIL_N = 0,
        TRAIL_R = 1,
        TSA_N = 2,
        TSA_R = 3,
        STSA_N = 4,
        STSA_R = 5,
        RADL_N = 6,
        RADL_R = 7,
        RASL_N = 8,
        RASL_R = 9,
        BLA_W_LP = 16,
        BLA_W_RADL = 17,
        BLA_N_LP = 18,
        IDR_W_RADL = 19,
        IDR_N_LP = 20,
        CRA_NUT = 21,
        RSV_IRAP_VCL22 = 22,
        RSV_IRAP_VCL23 = 23,
        VPS_NUT = 32,
        SPS_NUT = 33,
        PPS_NUT = 34,
        AUD_NUT = 35,
        EOS_NUT = 36,
        EOB_NUT = 37,
        FD_NUT = 38,
        PREFIX_SEI_NUT = 39,
        SUFFIX_SEI_NUT = 40,
    };

    struct TAvcSpsParsed {
        ui32 SpsID = 0;

        ui32 pic_height_in_map_units = 0;
        ui32 pic_width_in_mbs = 0;
        ui32 pic_order_cnt_type = 0;
        ui32 log2_max_pic_order_cnt_lsb = 0;
        ui32 log2_max_frame_num = 0;
        ui32 chroma_array_type = 0;
        ui32 chroma_format_idc = 0;
        ui32 frame_mbs_only_flag : 1 = 0;
        ui32 delta_pic_order_always_zero_flag : 1 = 0;
        ui32 separate_colour_plane_flag : 1 = 0;
    };

    struct TAvcPpsParsed {
        ui32 PpsID = 0;
        ui32 SpsID = 0;

        ui32 slice_group_change_rate = 0;
        ui32 num_ref_idx[2] = {0, 0}; // = {num_ref_idx_l0_default_active_minus1 + 1, num_ref_idx_l1_default_active_minus1 + 1}
        ui32 slice_group_map_type = 0;
        ui32 num_slice_groups_minus1 = 0;
        ui32 weighted_bipred_idc = 0;
        ui32 weighted_pred_flag : 1 = 0;
        ui32 deblocking_filter_control_present_flag : 1 = 0;
        ui32 redundant_pic_cnt_present_flag : 1 = 0;
        ui32 entropy_coding_mode_flag : 1 = 0;
        ui32 bottom_field_pic_order_in_frame_present_flag : 1 = 0;
    };

    struct THevcShortTermRPS {
        struct TDeltaPocUsedByCurrPic {
            i32 DeltaPoc;
            bool UsedByCurrPic;
        };

        static constexpr int MaxRefPicsCount = 16;

        ui32 NumDeltaPocs;    // == NumNegativePics + NumPositivePics
        ui32 NumNegativePics; // count of S0 entries
        ui32 NumPositivePics; // count of S1 entries

        TDeltaPocUsedByCurrPic S0[MaxRefPicsCount]; // values of DeltaPocS0 and UsedByCurrPicS0
        TDeltaPocUsedByCurrPic S1[MaxRefPicsCount]; // values of DeltaPocS1 and UsedByCurrPicS1
    };

    struct THevcSpsParsed {
        ui32 SpsID;

        ui32 log2_min_luma_coding_block_size = 0;
        ui32 log2_diff_max_min_luma_coding_block_size = 0;
        ui32 pic_width_in_luma_samples = 0;
        ui32 pic_height_in_luma_samples = 0;
        ui32 log2_max_pic_order_cnt_lsb = 0;
        ui32 num_short_term_ref_pic_sets = 0;
        ui32 num_long_term_ref_pics_sps = 0;
        ui32 used_by_curr_pic_lt_sps_flags = 0; // bitmask of used_by_curr_pic_lt_sps_flag[i]
        ui32 bit_depth_luma = 0;
        ui32 bit_depth_chroma = 0;
        TVector<THevcShortTermRPS> st_rps; // short_term_ref_pic_sets

        ui32 sps_max_sub_layers_minus1 = 0;
        ui32 chroma_format_idc = 0;
        ui32 motion_vector_resolution_control_idc = 0;
        ui32 separate_colour_plane_flag : 1 = 0;
        ui32 sample_adaptive_offset_enabled_flag : 1 = 0;
        ui32 long_term_ref_pics_present_flag : 1 = 0;
        ui32 sps_temporal_mvp_enabled_flag : 1 = 0;
    };

    struct THevcPpsParsed {
        ui32 PpsID;
        ui32 SpsID;

        ui32 num_ref_idx[2] = {0, 0}; // = {num_ref_idx_l0_default_active_minus1 + 1, num_ref_idx_l1_default_active_minus1 + 1}

        ui32 num_extra_slice_header_bits = 0;
        ui32 slice_segment_header_extension_present_flag : 1 = 0;
        ui32 pps_loop_filter_across_slices_enabled_flag : 1 = 0;
        ui32 pps_slice_chroma_qp_offsets_present_flag : 1 = 0;
        ui32 deblocking_filter_override_enabled_flag : 1 = 0;
        ui32 pps_slice_act_qp_offsets_present_flag : 1 = 0;
        ui32 dependent_slice_segments_enabled_flag : 1 = 0;
        ui32 pps_deblocking_filter_disabled_flag : 1 = 0;
        ui32 chroma_qp_offset_list_enabled_flag : 1 = 0;
        ui32 entropy_coding_sync_enabled_flag : 1 = 0;
        ui32 lists_modification_present_flag : 1 = 0;
        ui32 pps_curr_pic_ref_enabled_flag : 1 = 0;
        ui32 output_flag_present_flag : 1 = 0;
        ui32 cabac_init_present_flag : 1 = 0;
        ui32 weighted_bipred_flag : 1 = 0;
        ui32 weighted_pred_flag : 1 = 0;
        ui32 tiles_enabled_flag : 1 = 0;
    };

    TAvcSpsParsed ParseAvcSps(TArrayRef<const ui8> spsData);
    TAvcPpsParsed ParseAvcPps(TArrayRef<const ui8> ppsData, const TVector<TAvcSpsParsed>& sps);

    THevcSpsParsed ParseHevcSps(TArrayRef<const ui8> spsData);
    THevcPpsParsed ParseHevcPps(TArrayRef<const ui8> ppsData);

    // parse nal type byte(s)
    // if nal has no slice data (non-VCL type) return {}
    // otherwise parse-skip slice header and return offset to first complete byte of slice data
    TMaybe<ui32> GetSliceDataOffsetAvc(
        ui8 const* const data,
        const size_t dataSize,
        TVector<TAvcSpsParsed>& spsVector,
        TVector<TAvcPpsParsed>& ppsVector);
    TMaybe<ui32> GetSliceDataOffsetHevc(
        ui8 const* const data,
        const size_t dataSize,
        TVector<THevcSpsParsed>& spsVector,  // if got sps nal parse it and add to that vector
        TVector<THevcPpsParsed>& ppsVector); // if got pps nal parse it and add to that vector

    // read 'ue(v)' - unsigned integer Exp-Golomb-coded syntax element, described in ISO 14496-10
    template <typename TByteReader>
    inline ui32 ReadUEV(NMP4Muxer::TBitReader<TByteReader>& reader) {
        int zeroBits = 0;
        while (reader.ReadBit() == 0) {
            ++zeroBits;
        }
        Y_ENSURE(zeroBits < 32);
        return (ui32(1) << zeroBits) - 1 + reader.template Read<ui32>(zeroBits);
    }

    // read 'se(v)' - signed integer Exp-Golomb-coded syntax element, described in ISO 14496-10
    template <typename TByteReader>
    inline i32 ReadSEV(NMP4Muxer::TBitReader<TByteReader>& reader) {
        ui32 v = ReadUEV<TByteReader>(reader);
        if (v & 1) {
            return v / 2 + 1;
        } else {
            return -i32(v / 2);
        }
    }

    // class to use as TByteReader for TBitReader
    // to read bits directly from Avc/Hevc RBSP data doing emulation prevention decode on demand
    class TEmulationPreventionDecodeByteReader {
    public:
        TEmulationPreventionDecodeByteReader(ui8 const* const data, size_t size)
            : SimpleByteReader(data, size)
            , ZerosCount(0)
        {
        }

        size_t GetPosition() const {
            return SimpleByteReader.GetPosition();
        }

        ui8 ReadByte() {
            const ui8 byte = SimpleByteReader.ReadByte();
            if (byte == 0) {
                ++ZerosCount;
                return byte;
            } else if (ZerosCount >= 2 && byte == 3) {
                const ui8 newxByte = SimpleByteReader.ReadByte();
                ZerosCount = newxByte == 0 ? 1 : 0;
                return newxByte;
            } else {
                ZerosCount = 0;
                return byte;
            }
        }

    private:
        NMP4Muxer::TSimpleByteReader SimpleByteReader;
        size_t ZerosCount;
    };

    template <typename TVec>
    inline decltype(&std::declval<TVec>()[0]) FindSps(TVec& spsVector, const ui32 spsID) {
        for (auto& sps : spsVector) {
            if (sps.SpsID == spsID) {
                return &sps;
            }
        }
        return nullptr;
    }

    template <typename TVec>
    inline decltype(&std::declval<TVec>()[0]) FindPps(TVec& ppsVector, const ui32 ppsID) {
        for (auto& pps : ppsVector) {
            if (pps.PpsID == ppsID) {
                return &pps;
            }
        }
        return nullptr;
    }

}
