#include <nginx/modules/strm_packager/src/common/h2645_util_common.h>

#include <nginx/modules/strm_packager/src/common/math.h>

#include <util/generic/algorithm.h>

namespace NStrm::NPackager::Nh2645 {
    static bool CheckRBSPTrailingBitsAvc(TEPDBitReader& reader) {
        return CheckByteAligmentBits(reader);
    }

    static void SkipScalingListAvc(TEPDBitReader& reader, const size_t size) {
        int lastScale = 8;
        int nextScale = 8;

        for (size_t j = 0; j < size; ++j) {
            if (nextScale != 0) {
                const int deltaScale = ReadSEV(reader);
                nextScale = (lastScale + deltaScale) & 0xff;
            }
            lastScale = (nextScale == 0) ? lastScale : nextScale;
        }
    }

    static void SkipHRDParametersAvc(TEPDBitReader& reader) {
        const size_t cpb_cnt = ReadUEV(reader) + 1;
        reader.Read<ui8, 4>(); // = bit_rate_scale
        reader.Read<ui8, 4>(); // = cpb_size_scale
        for (size_t schedSelIdx = 0; schedSelIdx < cpb_cnt; ++schedSelIdx) {
            ReadUEV(reader);  // = bit_rate_value_minus1[schedSelIdx]
            ReadUEV(reader);  // = cpb_size_value_minus1[schedSelIdx]
            reader.ReadBit(); // = cbr_flag[SchedSelIdx]
        }
        reader.Read<ui8, 5>(); // = initial_cpb_removal_delay_length_minus1
        reader.Read<ui8, 5>(); // = cpb_removal_delay_length_minus1
        reader.Read<ui8, 5>(); // = dpb_output_delay_length_minus1
        reader.Read<ui8, 5>(); // = time_offset_length
    }

    static void SkipVUIParametersAvc(TEPDBitReader& reader) {
        if (reader.ReadBit()) {                                 // = aspect_ratio_info_present_flag
            const ui8 aspect_ratio_idc = reader.Read<ui8, 8>(); // = aspect_ratio_idc
            if (aspect_ratio_idc == 255) {                      // = EXTENDED_SAR == 255
                reader.Read<ui16, 16>();                        // = sar_width
                reader.Read<ui16, 16>();                        // = sar_height
            }
        }

        if (reader.ReadBit()) { // = overscan_info_present_flag
            reader.ReadBit();   // = overscan_appropriate_flag
        }

        if (reader.ReadBit()) {     // = video_signal_type_present_flag
            reader.Read<ui32, 3>(); // = video_format
            reader.ReadBit();       // = video_full_range_flag

            if (reader.ReadBit()) {     // = colour_description_present_flag
                reader.Read<ui32, 8>(); // = colour_primaries
                reader.Read<ui32, 8>(); // = transfer_characteristics
                reader.Read<ui32, 8>(); // = matrix_coefficients
            }
        }

        if (reader.ReadBit()) { // = chroma_loc_info_present_flag
            ReadUEV(reader);    // = chroma_sample_loc_type_top_field
            ReadUEV(reader);    // = chroma_sample_loc_type_bottom_field
        }

        if (reader.ReadBit()) {      // = timing_info_present_flag
            reader.Read<ui32, 32>(); // = num_units_in_tick
            reader.Read<ui32, 32>(); // = time_scale
            reader.ReadBit();        // = fixed_frame_rate_flag
        }

        const bool nal_hrd_parameters_present_flag = reader.ReadBit();
        if (nal_hrd_parameters_present_flag) {
            SkipHRDParametersAvc(reader);
        }

        const bool vcl_hrd_parameters_present_flag = reader.ReadBit();
        if (vcl_hrd_parameters_present_flag) {
            SkipHRDParametersAvc(reader);
        }

        if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
            reader.ReadBit(); // = low_delay_hrd_flag
        }

        reader.ReadBit(); // = pic_struct_present_flag

        if (reader.ReadBit()) { // = bitstream_restriction_flag
            reader.ReadBit();   // = motion_vectors_over_pic_boundaries_flag
            ReadUEV(reader);    // = max_bytes_per_pic_denom
            ReadUEV(reader);    // = max_bits_per_mb_denom
            ReadUEV(reader);    // = log2_max_mv_length_horizontal
            ReadUEV(reader);    // = log2_max_mv_length_vertical
            ReadUEV(reader);    // = num_reorder_frames
            ReadUEV(reader);    // = max_dec_frame_buffering
        }
    }

    // table 'Sequence parameter set RBSP syntax' of ISO 14496-10
    TAvcSpsParsed ParseAvcSps(const TArrayRef<const ui8> spsData) {
        TAvcSpsParsed sps;

        TEPDBitReader reader(spsData.data(), spsData.size());

        reader.ReadBit();      // = forbidden_zero_bit
        reader.Read<ui8, 2>(); // = nal_ref_idc
        const EAvcNalType nalType = (EAvcNalType)reader.Read<ui32, 5>();
        Y_ENSURE(nalType == EAvcNalType::SPS);

        const ui8 profileIdc = reader.Read<ui8, 8>();
        reader.Read<ui8, 8>();       // = skip constraint set flag 0-5 and 2 rezerved bits
        reader.Read<ui8, 8>();       // = level_idc
        sps.SpsID = ReadUEV(reader); // = seq_parameter_set_id

        static const ui8 profilesIdcWithChromaBlock[] = {44, 83, 86, 100, 110, 118, 122, 128, 134, 135, 138, 139, 244};
        if (FindPtr(profilesIdcWithChromaBlock, profileIdc)) {
            sps.chroma_format_idc = ReadUEV(reader);
            sps.chroma_array_type = sps.chroma_format_idc;
            if (sps.chroma_format_idc == 3) {
                sps.separate_colour_plane_flag = reader.ReadBit();
                if (sps.separate_colour_plane_flag) {
                    sps.chroma_array_type = 0;
                }
            }

            ReadUEV(reader);  // = bit_depth_luma_minus8
            ReadUEV(reader);  // = bit_depth_chroma_minus8
            reader.ReadBit(); // = qpprime_y_zero_transform_bypass_flag

            if (reader.ReadBit()) { // = seq_scaling_matrix_present_flag
                const int count = sps.chroma_format_idc == 3 ? 12 : 8;
                for (int i = 0; i < count; ++i) {
                    if (reader.ReadBit()) { // = seq_scaling_list_present_flag
                        SkipScalingListAvc(reader, i < 6 ? 16 : 64);
                    }
                }
            }
        } else {
            sps.chroma_format_idc = 1;
            sps.chroma_array_type = 1;
        }

        sps.log2_max_frame_num = ReadUEV(reader) + 4;
        sps.pic_order_cnt_type = ReadUEV(reader);

        if (sps.pic_order_cnt_type == 0) {
            sps.log2_max_pic_order_cnt_lsb = ReadUEV(reader) + 4;
        } else if (sps.pic_order_cnt_type == 1) {
            sps.delta_pic_order_always_zero_flag = reader.ReadBit();
            ReadSEV(reader); // = offset_for_non_ref_pic
            ReadSEV(reader); // = offset_for_top_to_bottom_field

            const int count = ReadUEV(reader); // = num_ref_frames_in_pic_order_cnt_cycle

            for (int i = 0; i < count; ++i) {
                ReadSEV(reader); // = offset_for_ref_frame[i]
            }
        }

        ReadUEV(reader);  // = num_ref_frames
        reader.ReadBit(); // = gaps_in_frame_num_value_allowed_flag
        sps.pic_width_in_mbs = ReadUEV(reader) + 1;
        sps.pic_height_in_map_units = ReadUEV(reader) + 1;
        sps.frame_mbs_only_flag = reader.ReadBit();

        if (!sps.frame_mbs_only_flag) {
            reader.ReadBit(); // = mb_adaptive_frame_field_flag
        }

        reader.ReadBit(); // = direct_8x8_inference_flag

        if (reader.ReadBit()) { // = frame_cropping_flag
            ReadUEV(reader);    // = frame_crop_left_offset
            ReadUEV(reader);    // = frame_crop_right_offset
            ReadUEV(reader);    // = frame_crop_top_offset
            ReadUEV(reader);    // = frame_crop_bottom_offset
        }
        if (reader.ReadBit()) { // = vui_parameters_present_flag
            SkipVUIParametersAvc(reader);
        }

        Y_ENSURE(CheckRBSPTrailingBitsAvc(reader) && reader.GetPosition() == spsData.size());

        return sps;
    }

    // table 'Picture parameter set RBSP syntax' of ISO 14496-10
    TAvcPpsParsed ParseAvcPps(TArrayRef<const ui8> ppsData, const TVector<TAvcSpsParsed>& spsVector) {
        TAvcPpsParsed pps;

        TEPDBitReader reader(ppsData.data(), ppsData.size());

        reader.ReadBit();      // = forbidden_zero_bit
        reader.Read<ui8, 2>(); // = nal_ref_idc
        const EAvcNalType nalType = (EAvcNalType)reader.Read<ui32, 5>();
        Y_ENSURE(nalType == EAvcNalType::PPS);

        pps.PpsID = ReadUEV(reader);
        pps.SpsID = ReadUEV(reader);

        TAvcSpsParsed const* const spsPtr = FindSps(spsVector, pps.SpsID);
        Y_ENSURE(spsPtr, "sps id " << pps.SpsID << " not found at ParseAvcPps");
        const TAvcSpsParsed& sps = *spsPtr;

        pps.entropy_coding_mode_flag = reader.ReadBit();
        pps.bottom_field_pic_order_in_frame_present_flag = reader.ReadBit();
        pps.num_slice_groups_minus1 = ReadUEV(reader);
        if (pps.num_slice_groups_minus1 > 0) {
            pps.slice_group_map_type = ReadUEV(reader);
            if (pps.slice_group_map_type == 0) {
                for (ui32 group = 0; group <= pps.num_slice_groups_minus1; ++group) {
                    ReadUEV(reader); // = run_length_minus1[group]
                }
            } else if (pps.slice_group_map_type == 2) {
                for (ui32 group = 0; group < pps.num_slice_groups_minus1; ++group) {
                    ReadUEV(reader); // = top_left[ group ]
                    ReadUEV(reader); // = bottom_right[ group ]
                }
            } else if (pps.slice_group_map_type == 3 || pps.slice_group_map_type == 4 || pps.slice_group_map_type == 5) {
                reader.ReadBit(); // = slice_group_change_direction_flag
                pps.slice_group_change_rate = ReadUEV(reader) + 1;
            } else if (pps.slice_group_map_type == 6) {
                const ui32 pic_size_in_map_units_minus1 = ReadUEV(reader);
                const ui32 slice_group_id_bitsize = CeilLog2(pps.num_slice_groups_minus1 + 1);
                for (ui32 i = 0; i <= pic_size_in_map_units_minus1; ++i) {
                    reader.Read<ui32>(slice_group_id_bitsize); // = slice_group_id[ i ]
                }
            }
        }

        pps.num_ref_idx[0] = ReadUEV(reader) + 1;
        pps.num_ref_idx[1] = ReadUEV(reader) + 1;
        pps.weighted_pred_flag = reader.ReadBit();
        pps.weighted_bipred_idc = reader.Read<ui8, 2>();
        ReadSEV(reader); // = pic_init_qp_minus26
        ReadSEV(reader); // = pic_init_qs_minus26
        ReadSEV(reader); // = chroma_qp_index_offset
        pps.deblocking_filter_control_present_flag = reader.ReadBit();
        reader.ReadBit(); // = constrained_intra_pred_flag
        pps.redundant_pic_cnt_present_flag = reader.ReadBit();

        TEPDBitReader tmpReader = reader;
        if (CheckRBSPTrailingBitsAvc(tmpReader) && tmpReader.GetPosition() == ppsData.size()) {
            return pps;
        }

        const ui32 transform_8x8_mode_flag = reader.ReadBit();
        if (reader.ReadBit()) { // = pic_scaling_matrix_present_flag
            const ui32 count = 6 + ((sps.chroma_format_idc != 3) ? 2 : 6) * transform_8x8_mode_flag;
            for (ui32 i = 0; i < count; ++i) {
                if (reader.ReadBit()) { // = pic_scaling_list_present_flag
                    if (i < 6) {
                        SkipScalingListAvc(reader, 16);
                    } else {
                        SkipScalingListAvc(reader, 64);
                    }
                }
            }
        }

        ReadSEV(reader); // = second_chroma_qp_index_offset

        Y_ENSURE(CheckRBSPTrailingBitsAvc(reader) && reader.GetPosition() == ppsData.size());

        return pps;
    }

    static void SkipRefPicListMvcModificationAvc(TEPDBitReader& reader, const EAvcSlice sliceType) {
        if (sliceType != EAvcSlice::I && sliceType != EAvcSlice::SI) {
            if (reader.ReadBit()) { // = ref_pic_list_modification_flag_l0
                ui32 modification_of_pic_nums_idc;
                do {
                    modification_of_pic_nums_idc = ReadUEV(reader);
                    if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1) {
                        ReadUEV(reader); // = abs_diff_pic_num_minus1
                    } else if (modification_of_pic_nums_idc == 2) {
                        ReadUEV(reader); // = long_term_pic_num
                    } else if (modification_of_pic_nums_idc == 4 || modification_of_pic_nums_idc == 5) {
                        ReadUEV(reader); // = abs_diff_view_idx_minus1
                    }
                } while (modification_of_pic_nums_idc != 3);
            }
        }

        if (sliceType == EAvcSlice::B) {
            if (reader.ReadBit()) { // = ref_pic_list_modification_flag_l1
                ui32 modification_of_pic_nums_idc;
                do {
                    modification_of_pic_nums_idc = ReadUEV(reader);
                    if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1) {
                        ReadUEV(reader); // = abs_diff_pic_num_minus1
                    } else if (modification_of_pic_nums_idc == 2) {
                        ReadUEV(reader); // = long_term_pic_num
                    } else if (modification_of_pic_nums_idc == 4 || modification_of_pic_nums_idc == 5) {
                        ReadUEV(reader); // = abs_diff_view_idx_minus1
                    }
                } while (modification_of_pic_nums_idc != 3);
            }
        }
    }

    static void SkipRefPicListModificationAvc(TEPDBitReader& reader, const EAvcSlice sliceType) {
        if (sliceType != EAvcSlice::I && sliceType != EAvcSlice::SI) {
            if (reader.ReadBit()) { // = ref_pic_list_modification_flag_l0
                ui32 modification_of_pic_nums_idc;
                do {
                    modification_of_pic_nums_idc = ReadUEV(reader);
                    if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1) {
                        ReadUEV(reader); // = abs_diff_pic_num_minus1
                    } else if (modification_of_pic_nums_idc == 2) {
                        ReadUEV(reader); // = long_term_pic_num
                    }
                } while (modification_of_pic_nums_idc != 3);
            }
        }

        if (sliceType == EAvcSlice::B) {
            if (reader.ReadBit()) { // = ref_pic_list_modification_flag_l1
                ui32 modification_of_pic_nums_idc;
                do {
                    modification_of_pic_nums_idc = ReadUEV(reader);
                    if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1) {
                        ReadUEV(reader); // = abs_diff_pic_num_minus1
                    } else if (modification_of_pic_nums_idc == 2) {
                        ReadUEV(reader); // = long_term_pic_num
                    }
                } while (modification_of_pic_nums_idc != 3);
            }
        }
    }

    // table 'Prediction weight table syntax' of ISO 14496-10
    static void SkipPredWeightTableAvc(TEPDBitReader& reader, const EAvcSlice sliceType, const ui32 (&num_ref_idx)[2], const ui32 chroma_array_type) {
        ReadUEV(reader); // = luma_log2_weight_denom
        if (chroma_array_type != 0) {
            ReadUEV(reader); // = chroma_log2_weight_denom
        }

        for (ui32 i = 0; i < num_ref_idx[0]; ++i) {
            if (reader.ReadBit()) { // = luma_weight_l0_flag
                ReadSEV(reader);    // = luma_weight_l0[i]
                ReadSEV(reader);    // = luma_offset_l0[i]
            }
            if (chroma_array_type != 0) {
                if (reader.ReadBit()) { // = chroma_weight_l0_flag
                    for (int j = 0; j < 2; ++j) {
                        ReadSEV(reader); // = chroma_weight_l0[i][j]
                        ReadSEV(reader); // = chroma_offset_l0[i][j]
                    }
                }
            }
        }

        if (sliceType == EAvcSlice::B) {
            for (ui32 i = 0; i < num_ref_idx[1]; ++i) {
                if (reader.ReadBit()) { // = luma_weight_l1_flag
                    ReadSEV(reader);    // = luma_weight_l1[i]
                    ReadSEV(reader);    // = luma_offset_l1[i]
                }

                if (chroma_array_type != 0) {
                    if (reader.ReadBit()) { // = chroma_weight_l1_flag
                        for (int j = 0; j < 2; ++j) {
                            ReadSEV(reader); // = chroma_weight_l1[i][j]
                            ReadSEV(reader); // = chroma_offset_l1[i][j]
                        }
                    }
                }
            }
        }
    }

    static void SkipDecRefPicMarkingAvc(TEPDBitReader& reader, EAvcNalType nalType) {
        if (nalType == EAvcNalType::IDR_SLICE) {
            reader.ReadBit(); // = no_output_of_prior_pics_flag
            reader.ReadBit(); // = long_term_reference_flag
        } else {
            if (reader.ReadBit()) { // = adaptive_ref_pic_marking_mode_flag
                ui32 memory_management_control_operation;
                do {
                    memory_management_control_operation = ReadUEV(reader);
                    if (memory_management_control_operation == 1 || memory_management_control_operation == 3) {
                        ReadUEV(reader); // = difference_of_pic_nums_minus1
                    }
                    if (memory_management_control_operation == 2) {
                        ReadUEV(reader); // = long_term_pic_num
                    }
                    if (memory_management_control_operation == 3 || memory_management_control_operation == 6) {
                        ReadUEV(reader); // = long_term_frame_idx
                    }
                    if (memory_management_control_operation == 4) {
                        ReadUEV(reader); // = max_long_term_frame_idx_plus1
                    }
                } while (memory_management_control_operation != 0);
            }
        }
    }

    // table `Slice header syntax` of ISO 14496-10
    static void SkipSliceHeaderAvc(
        TEPDBitReader& reader,
        const ui8 nalRefIdc,
        const EAvcNalType nalType,
        const TVector<TAvcSpsParsed>& spsVector,
        const TVector<TAvcPpsParsed>& ppsVector) {
        ReadUEV(reader); // = first_mb_in_slice

        const ui32 sliceTypeOrig = ReadUEV(reader);
        Y_ENSURE(sliceTypeOrig < 10, "ivalid Avc slice type " << sliceTypeOrig);
        const EAvcSlice sliceType = EAvcSlice(sliceTypeOrig % 5);

        const ui32 ppsID = ReadUEV(reader);

        TAvcPpsParsed const* const ppsPtr = FindPps(ppsVector, ppsID);
        Y_ENSURE(ppsPtr, "pps id " << ppsID << " not found at SkipSliceHeaderAvc");
        const TAvcPpsParsed& pps = *ppsPtr;

        TAvcSpsParsed const* const spsPtr = FindSps(spsVector, pps.SpsID);
        Y_ENSURE(spsPtr, "sps id " << pps.SpsID << " not found at SkipSliceHeaderAvc");
        const TAvcSpsParsed& sps = *spsPtr;

        if (sps.separate_colour_plane_flag) {
            reader.Read<ui8, 2>(); // = colour_plane_id
        }
        reader.Read<ui32>(sps.log2_max_frame_num); // = frame_num

        bool field_pic_flag = false;
        if (!sps.frame_mbs_only_flag) {
            field_pic_flag = reader.ReadBit();
            if (field_pic_flag) {
                reader.ReadBit(); // = bottom_field_flag
            }
        }

        if (nalType == EAvcNalType::IDR_SLICE) {
            ReadSEV(reader); // = idr_pic_id
        }

        if (sps.pic_order_cnt_type == 0) {
            reader.Read<ui32>(sps.log2_max_pic_order_cnt_lsb); // = pic_order_cnt_lsb
            if (pps.bottom_field_pic_order_in_frame_present_flag && !field_pic_flag) {
                ReadSEV(reader); // = delta_pic_order_cnt_bottom
            }
        }

        if (sps.pic_order_cnt_type == 1 && !sps.delta_pic_order_always_zero_flag) {
            ReadSEV(reader); // = delta_pic_order_cnt[0]
            if (pps.bottom_field_pic_order_in_frame_present_flag && !field_pic_flag) {
                ReadSEV(reader); // = delta_pic_order_cnt[1]
            }
        }

        if (pps.redundant_pic_cnt_present_flag) {
            ReadUEV(reader); // = redundant_pic_cnt
        }

        if (sliceType == EAvcSlice::B) {
            reader.ReadBit(); // = direct_spatial_mv_pred_flag
        }

        // {num_ref_idx_l0_active_minus1 + 1, num_ref_idx_l1_active_minus1 + 1}
        ui32 num_ref_idx[2] = {pps.num_ref_idx[0], pps.num_ref_idx[1]};
        if (sliceType == EAvcSlice::P || sliceType == EAvcSlice::SP || sliceType == EAvcSlice::B) {
            if (reader.ReadBit()) { // = num_ref_idx_active_override_flag
                num_ref_idx[0] = ReadUEV(reader) + 1;
                if (sliceType == EAvcSlice::B) {
                    num_ref_idx[1] = ReadUEV(reader) + 1;
                }
            }
        }

        if (nalType == (EAvcNalType)20 || nalType == (EAvcNalType)21) {
            SkipRefPicListMvcModificationAvc(reader, sliceType);
        } else {
            SkipRefPicListModificationAvc(reader, sliceType);
        }

        if ((pps.weighted_pred_flag && (sliceType == EAvcSlice::P || sliceType == EAvcSlice::SP)) || (pps.weighted_bipred_idc == 1 && sliceType == EAvcSlice::B)) {
            SkipPredWeightTableAvc(reader, sliceType, num_ref_idx, sps.chroma_array_type);
        }

        if (nalRefIdc != 0) {
            SkipDecRefPicMarkingAvc(reader, nalType);
        }

        if (pps.entropy_coding_mode_flag && sliceType != EAvcSlice::I && sliceType != EAvcSlice::SI) {
            ReadUEV(reader); // = cabac_init_idc
        }

        ReadSEV(reader); // = slice_qp_delta

        if (sliceType == EAvcSlice::SP || sliceType == EAvcSlice::SI) {
            if (sliceType == EAvcSlice::SP) {
                reader.ReadBit(); // = sp_for_switch_flag
            }
            ReadSEV(reader); // = slice_qs_delta
        }

        if (pps.deblocking_filter_control_present_flag) {
            const ui32 disable_deblocking_filter_idc = ReadUEV(reader);
            if (disable_deblocking_filter_idc != 1) {
                ReadSEV(reader); // = slice_alpha_c0_offset_div2
                ReadSEV(reader); // = slice_beta_offset_div2
            }
        }

        if (pps.num_slice_groups_minus1 > 0 && pps.slice_group_map_type >= 3 && pps.slice_group_map_type <= 5) {
            const ui32 pic_size_in_map_units = sps.pic_height_in_map_units * sps.pic_width_in_mbs;
            const ui32 bitsize = CeilLog2(1 + DivCeil(pic_size_in_map_units, pps.slice_group_change_rate));
            reader.Read<ui32>(bitsize); // = slice_group_change_cycle
        }
    }

    TMaybe<ui32> GetSliceDataOffsetAvc(
        ui8 const* const data,
        const size_t dataSize,
        TVector<TAvcSpsParsed>& spsVector,
        TVector<TAvcPpsParsed>& ppsVector) {
        TEPDBitReader reader(data, dataSize);

        reader.ReadBit(); // = forbidden_zero_bit
        const ui8 nalRefIdc = reader.Read<ui8, 2>();
        const EAvcNalType nalType = (EAvcNalType)reader.Read<ui32, 5>();
        Y_ENSURE(nalType != EAvcNalType::UNSPECIFIED);

        // it is confusing what to do with DPA, DPB and DPC types
        //   since they all have slice data, but DPB and DPC have few numbers instead of slice header
        //   and DPA have few numbers between slice header and slice data
        // but CENC states as if it is always slice data right after slice header..
        // so here made the same way bento4 do it (have slice header == slice data start right after; no slice header == no slice data) :

        switch (nalType) {
            case EAvcNalType::SLICE:
            case EAvcNalType::IDR_SLICE:
            case EAvcNalType::DPA:
                SkipSliceHeaderAvc(reader, nalRefIdc, nalType, spsVector, ppsVector);
                return reader.GetPosition();

            case EAvcNalType::SPS: {
                TAvcSpsParsed sps = ParseAvcSps({data, dataSize});
                if (TAvcSpsParsed* tptr = FindSps(spsVector, sps.SpsID)) {
                    *tptr = std::move(sps);
                } else {
                    spsVector.push_back(std::move(sps));
                }
                return {};
            }

            case EAvcNalType::PPS: {
                TAvcPpsParsed pps = ParseAvcPps({data, dataSize}, spsVector);
                if (TAvcPpsParsed* tptr = FindPps(ppsVector, pps.PpsID)) {
                    *tptr = std::move(pps);
                } else {
                    ppsVector.push_back(std::move(pps));
                }
                return {};
            }

            case EAvcNalType::DPB:
            case EAvcNalType::DPC:
            default:
                return {};
        }
    }
}
