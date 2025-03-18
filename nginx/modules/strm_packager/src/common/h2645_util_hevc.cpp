#include <nginx/modules/strm_packager/src/common/h2645_util_common.h>

#include <nginx/modules/strm_packager/src/common/math.h>

#include <util/generic/algorithm.h>

namespace NStrm::NPackager::Nh2645 {
    // Short-term reference picture set syntax
    // st_ref_pic_set( stRpsIdx ) {
    static THevcShortTermRPS ParseShortTermRefPicSetHevc(TEPDBitReader& reader, const THevcSpsParsed& sps, const ui32 stRpsIdx) {
        static constexpr int MaxRefPicsCount = THevcShortTermRPS::MaxRefPicsCount;

        THevcShortTermRPS rps;

        bool inter_ref_pic_set_prediction_flag = false;
        if (stRpsIdx != 0) {
            inter_ref_pic_set_prediction_flag = reader.ReadBit();
        }

        if (inter_ref_pic_set_prediction_flag) {
            ui32 delta_idx = 1;
            if (stRpsIdx == sps.num_short_term_ref_pic_sets) {
                delta_idx = ReadUEV(reader) + 1;
                Y_ENSURE(delta_idx <= stRpsIdx);
            }

            const ui32 refRpsIdx = stRpsIdx - delta_idx;
            Y_ENSURE(refRpsIdx < sps.st_rps.size());
            const THevcShortTermRPS& refRPS = sps.st_rps[refRpsIdx];

            const bool delta_rps_sign = reader.ReadBit();
            const ui32 abs_delta_rps = ReadUEV(reader) + 1;
            Y_ENSURE(abs_delta_rps >= 1);
            Y_ENSURE(abs_delta_rps <= (1 << 15));

            const i32 delta_rps = (delta_rps_sign ? -1 : 1) * i32(abs_delta_rps);

            bool used_by_curr_pic_flag[refRPS.NumDeltaPocs + 1];
            bool use_delta_flag[refRPS.NumDeltaPocs + 1];

            for (size_t j = 0; j <= refRPS.NumDeltaPocs; ++j) {
                used_by_curr_pic_flag[j] = reader.ReadBit();

                use_delta_flag[j] = true;
                if (!used_by_curr_pic_flag[j]) {
                    use_delta_flag[j] = reader.ReadBit();
                }
            }

            rps.NumNegativePics = 0;
            for (i32 j = refRPS.NumPositivePics - 1; j >= 0; --j) {
                const i32 dPoc = refRPS.S1[j].DeltaPoc + delta_rps;
                if (dPoc < 0 && use_delta_flag[refRPS.NumNegativePics + j]) {
                    Y_ENSURE(rps.NumNegativePics < MaxRefPicsCount);
                    rps.S0[rps.NumNegativePics].DeltaPoc = dPoc;
                    rps.S0[rps.NumNegativePics].UsedByCurrPic = used_by_curr_pic_flag[refRPS.NumNegativePics + j];
                    ++rps.NumNegativePics;
                }
            }
            if (delta_rps < 0 && use_delta_flag[refRPS.NumDeltaPocs]) {
                Y_ENSURE(rps.NumNegativePics < MaxRefPicsCount);
                rps.S0[rps.NumNegativePics].DeltaPoc = delta_rps;
                rps.S0[rps.NumNegativePics].UsedByCurrPic = used_by_curr_pic_flag[refRPS.NumDeltaPocs];
                ++rps.NumNegativePics;
            }
            for (ui32 j = 0; j < refRPS.NumNegativePics; ++j) {
                const i32 dPoc = refRPS.S0[j].DeltaPoc + delta_rps;
                if (dPoc < 0 && use_delta_flag[j]) {
                    Y_ENSURE(rps.NumNegativePics < MaxRefPicsCount);
                    rps.S0[rps.NumNegativePics].DeltaPoc = dPoc;
                    rps.S0[rps.NumNegativePics].UsedByCurrPic = used_by_curr_pic_flag[j];
                    ++rps.NumNegativePics;
                }
            }

            rps.NumPositivePics = 0;
            for (i32 j = refRPS.NumNegativePics - 1; j >= 0; --j) {
                const i32 dPoc = refRPS.S0[j].DeltaPoc + delta_rps;
                if (dPoc > 0 && use_delta_flag[j]) {
                    Y_ENSURE(rps.NumPositivePics < MaxRefPicsCount);
                    rps.S1[rps.NumPositivePics].DeltaPoc = dPoc;
                    rps.S1[rps.NumPositivePics].UsedByCurrPic = used_by_curr_pic_flag[j];
                    ++rps.NumPositivePics;
                }
            }
            if (delta_rps > 0 && use_delta_flag[refRPS.NumDeltaPocs]) {
                Y_ENSURE(rps.NumPositivePics < MaxRefPicsCount);
                rps.S1[rps.NumPositivePics].DeltaPoc = delta_rps;
                rps.S1[rps.NumPositivePics].UsedByCurrPic = used_by_curr_pic_flag[refRPS.NumDeltaPocs];
                ++rps.NumPositivePics;
            }
            for (ui32 j = 0; j < refRPS.NumPositivePics; ++j) {
                const i32 dPoc = refRPS.S1[j].DeltaPoc + delta_rps;
                if (dPoc > 0 && use_delta_flag[refRPS.NumNegativePics + j]) {
                    Y_ENSURE(rps.NumPositivePics < MaxRefPicsCount);
                    rps.S1[rps.NumPositivePics].DeltaPoc = dPoc;
                    rps.S1[rps.NumPositivePics].UsedByCurrPic = used_by_curr_pic_flag[refRPS.NumNegativePics + j];
                    ++rps.NumPositivePics;
                }
            }

            rps.NumDeltaPocs = rps.NumNegativePics + rps.NumPositivePics;
        } else {
            rps.NumNegativePics = ReadUEV(reader);
            rps.NumPositivePics = ReadUEV(reader);
            rps.NumDeltaPocs = rps.NumNegativePics + rps.NumPositivePics;

            Y_ENSURE(rps.NumNegativePics <= MaxRefPicsCount);
            Y_ENSURE(rps.NumPositivePics <= MaxRefPicsCount);

            for (ui32 i = 0; i < rps.NumNegativePics; ++i) {
                const i32 delta_poc_s0_minus1__i = ReadUEV(reader); // = delta_poc_s0_minus1[i]
                rps.S0[i].DeltaPoc = (i == 0 ? 0 : rps.S0[i - 1].DeltaPoc) - (delta_poc_s0_minus1__i + 1);
                rps.S0[i].UsedByCurrPic = reader.ReadBit(); // = used_by_curr_pic_s0_flag[i]
            }

            for (ui32 i = 0; i < rps.NumPositivePics; ++i) {
                const i32 delta_poc_s1_minus1__i = ReadUEV(reader);
                rps.S1[i].DeltaPoc = (i == 0 ? 0 : rps.S1[i - 1].DeltaPoc) + (delta_poc_s1_minus1__i + 1);
                rps.S1[i].UsedByCurrPic = reader.ReadBit(); // = used_by_curr_pic_s1_flag[i]
            }
        }

        return rps;
    }

    static ui32 GetShortTermUsedPicsCountHevc(const THevcShortTermRPS& strps) {
        ui32 result = 0;

        for (ui32 i = 0; i < strps.NumNegativePics; ++i) {
            if (strps.S0[i].UsedByCurrPic) {
                ++result;
            }
        }

        for (ui32 i = 0; i < strps.NumPositivePics; ++i) {
            if (strps.S1[i].UsedByCurrPic) {
                ++result;
            }
        }

        return result;
    }

    // Reference picture list modification syntax
    // ref_pic_lists_modification( )
    static void SkipRefPicListsModificationHevc(TEPDBitReader& reader, const EHevcSlice sliceType, const ui32 (&num_ref_idx)[2], const ui32 numPicTotalCurr) {
        const ui32 list_entry_bitsize = CeilLog2(numPicTotalCurr);

        if (reader.ReadBit()) { // = ref_pic_list_modification_flag_l0
            for (ui32 i = 0; i < num_ref_idx[0]; ++i) {
                const ui32 l = reader.Read<ui32>(list_entry_bitsize); // = list_entry_l0[i]
                Y_ENSURE(l < numPicTotalCurr);
            }
        }

        if (sliceType == EHevcSlice::B) {
            if (reader.ReadBit()) { // = ref_pic_list_modification_flag_l1
                for (ui32 i = 0; i < num_ref_idx[1]; ++i) {
                    const ui32 l = reader.Read<ui32>(list_entry_bitsize); // = list_entry_l1[i]
                    Y_ENSURE(l < numPicTotalCurr);
                }
            }
        }
    }

    // Weighted prediction parameters syntax
    // pred_weight_table( )
    static void SkipPredWeightTableHevc(TEPDBitReader& reader, const EHevcSlice slice_type, const ui32 (&num_ref_idx)[2], const ui32 chroma_array_type) {
        ReadUEV(reader); // = luma_log2_weight_denom
        if (chroma_array_type != 0) {
            ReadSEV(reader); // = delta_chroma_log2_weight_denom
        }

        bool luma_weight_l0_flag[Max(1u, num_ref_idx[0])];
        memset(luma_weight_l0_flag, 0, sizeof(luma_weight_l0_flag));
        for (ui32 i = 0; i < num_ref_idx[0]; ++i) {
            luma_weight_l0_flag[i] = reader.ReadBit();
        }

        bool chroma_weight_l0_flag[Max(1u, num_ref_idx[0])];
        memset(chroma_weight_l0_flag, 0, sizeof(chroma_weight_l0_flag));
        if (chroma_array_type != 0) {
            for (ui32 i = 0; i < num_ref_idx[0]; ++i) {
                chroma_weight_l0_flag[i] = reader.ReadBit();
            }
        }

        for (ui32 i = 0; i < num_ref_idx[0]; ++i) {
            if (luma_weight_l0_flag[i]) {
                ReadSEV(reader); // = delta_luma_weight_l0[i]
                ReadSEV(reader); // = luma_offset_l0[i]
            }

            if (chroma_weight_l0_flag[i]) {
                for (int j = 0; j < 2; ++j) {
                    ReadSEV(reader); // = delta_chroma_weight_l0[i][j]
                    ReadSEV(reader); // = delta_chroma_offset_l0[i][j]
                }
            }
        }

        if (slice_type == EHevcSlice::B) {
            bool luma_weight_l1_flag[Max(1u, num_ref_idx[1])];
            memset(luma_weight_l1_flag, 0, sizeof(luma_weight_l1_flag));
            for (ui32 i = 0; i < num_ref_idx[1]; ++i) {
                luma_weight_l1_flag[i] = reader.ReadBit();
            }

            bool chroma_weight_l1_flag[Max(1u, num_ref_idx[1])];
            memset(chroma_weight_l1_flag, 0, sizeof(chroma_weight_l1_flag));
            if (chroma_array_type != 0) {
                for (ui32 i = 0; i < num_ref_idx[1]; ++i) {
                    chroma_weight_l1_flag[i] = reader.ReadBit();
                }
            }

            for (ui32 i = 0; i < num_ref_idx[1]; ++i) {
                if (luma_weight_l1_flag[i]) {
                    ReadSEV(reader); // = delta_luma_weight_l1[i]
                    ReadSEV(reader); // = luma_offset_l1[i]
                }

                if (chroma_weight_l1_flag[i]) {
                    for (int j = 0; j < 2; ++j) {
                        ReadSEV(reader); // = delta_chroma_weight_l1[i][j]
                        ReadSEV(reader); // = delta_chroma_offset_l1[i][j]
                    }
                }
            }
        }
    }

    // sps_range_extension( )
    static void SkipSpsRangeExtensionHevc(TEPDBitReader& reader) {
        reader.ReadBit(); // = transform_skip_rotation_enabled_flag
        reader.ReadBit(); // = transform_skip_context_enabled_flag
        reader.ReadBit(); // = implicit_rdpcm_enabled_flag
        reader.ReadBit(); // = explicit_rdpcm_enabled_flag
        reader.ReadBit(); // = extended_precision_processing_flag
        reader.ReadBit(); // = intra_smoothing_disabled_flag
        reader.ReadBit(); // = high_precision_offsets_enabled_flag
        reader.ReadBit(); // = persistent_rice_adaptation_enabled_flag
        reader.ReadBit(); // = cabac_bypass_alignment_enabled_flag
    }

    // sps_multilayer_extension( )
    static void SkipSpsMultilayerExtensionHevc(TEPDBitReader& reader) {
        reader.ReadBit(); // = inter_view_mv_vert_constraint_flag
    }

    // sps_3d_extension( )
    static void SkipSps3dExtensionHevc(TEPDBitReader& reader) {
        for (ui32 d = 0; d <= 1; ++d) {
            reader.ReadBit(); // = iv_di_mc_enabled_flag[d]
            reader.ReadBit(); // = iv_mv_scal_enabled_flag[d]
            if (d == 0) {
                ReadUEV(reader);  // = log2_ivmc_sub_pb_size_minus3[d]
                reader.ReadBit(); // = iv_res_pred_enabled_flag[d]
                reader.ReadBit(); // = depth_ref_enabled_flag[d]
                reader.ReadBit(); // = vsp_mc_enabled_flag[d]
                reader.ReadBit(); // = dbbp_enabled_flag[d]
            } else {
                reader.ReadBit(); // = tex_mc_enabled_flag[d]
                ReadUEV(reader);  // = log2_texmc_sub_pb_size_minus3[d]
                reader.ReadBit(); // = intra_contour_enabled_flag[d]
                reader.ReadBit(); // = intra_dc_only_wedge_enabled_flag[d]
                reader.ReadBit(); // = cqt_cu_part_pred_enabled_flag[d]
                reader.ReadBit(); // = inter_dc_only_enabled_flag[d]
                reader.ReadBit(); // = skip_intra_enabled_flag[d]
            }
        }
    }

    // sps_scc_extension( )
    static void ParseSpsSccExtensionHevc(TEPDBitReader& reader, THevcSpsParsed& sps) {
        reader.ReadBit();       // = sps_curr_pic_ref_enabled_flag
        if (reader.ReadBit()) { // = palette_mode_enabled_flag
            ReadUEV(reader);    // = palette_max_size
            ReadUEV(reader);    // = delta_palette_max_predictor_size

            if (reader.ReadBit()) { // = sps_palette_predictor_initializers_present_flag
                const ui32 sps_num_palette_predictor_initializers_minus1 = ReadUEV(reader);
                const ui32 numComps = (sps.chroma_format_idc == 0) ? 1 : 3;

                for (ui32 comp = 0; comp < numComps; ++comp) {
                    for (ui32 i = 0; i <= sps_num_palette_predictor_initializers_minus1; ++i) {
                        reader.Read<ui32>(comp == 0 ? sps.bit_depth_luma : sps.bit_depth_chroma); // = sps_palette_predictor_initializer[comp][i]
                    }
                }
            }
        }
        sps.motion_vector_resolution_control_idc = reader.Read<ui32, 2>();
        reader.ReadBit(); // = intra_boundary_filtering_disabled_flag
    }

    // profile_tier_level( profilePresentFlag, maxNumSubLayersMinus1 )
    static void SkipProfileTierLevelHevc(TEPDBitReader& reader, const bool profilePresentFlag, const ui32 maxNumSubLayersMinus1) {
        if (profilePresentFlag) {
            reader.Read<ui32, 2>();  // = general_profile_space
            reader.ReadBit();        // = general_tier_flag
            reader.Read<ui32, 5>();  // = general_profile_idc
            reader.Read<ui32, 32>(); // = general_profile_compatibility_flag[j], j=0..31
            reader.ReadBit();        // = general_progressive_source_flag
            reader.ReadBit();        // = general_interlaced_source_flag
            reader.ReadBit();        // = general_non_packed_constraint_flag
            reader.ReadBit();        // = general_frame_only_constraint_flag
            reader.Read<ui64, 43>(); // = general_reserved_zero_43bits
            reader.ReadBit();        // = general_reserved_zero_bit
        }

        reader.Read<ui32, 8>(); // = general_level_idc

        bool sub_layer_profile_present_flag[Max(1u, maxNumSubLayersMinus1)];
        bool sub_layer_level_present_flag[Max(1u, maxNumSubLayersMinus1)];
        for (ui32 i = 0; i < maxNumSubLayersMinus1; ++i) {
            sub_layer_profile_present_flag[i] = reader.ReadBit();
            sub_layer_level_present_flag[i] = reader.ReadBit();
        }

        if (maxNumSubLayersMinus1 > 0) {
            for (ui32 i = maxNumSubLayersMinus1; i < 8; ++i) {
                reader.Read<ui32, 2>(); // = reserved_zero_2bits[i]
            }
        }

        for (ui32 i = 0; i < maxNumSubLayersMinus1; ++i) {
            if (sub_layer_profile_present_flag[i]) {
                reader.Read<ui32, 2>();  // = sub_layer_profile_space[i]
                reader.ReadBit();        // = sub_layer_tier_flag[i]
                reader.Read<ui32, 5>();  // = sub_layer_profile_idc[i]
                reader.Read<ui32, 32>(); // = sub_layer_profile_compatibility_flag[i][j], j=0..31
                reader.ReadBit();        // = sub_layer_progressive_source_flag[i]
                reader.ReadBit();        // = sub_layer_interlaced_source_flag[i]
                reader.ReadBit();        // = sub_layer_non_packed_constraint_flag[i]
                reader.ReadBit();        // = sub_layer_frame_only_constraint_flag[i]
                reader.Read<ui64, 43>(); // = sub_layer_reserved_zero_43bits[i]
                reader.ReadBit();        // = sub_layer_reserved_zero_bit[i]
            }

            if (sub_layer_level_present_flag[i]) {
                reader.Read<ui32, 8>(); // = sub_layer_level_idc[i]
            }
        }
    }

    // scaling_list_data( )
    static void SkipScalingListDataHevc(TEPDBitReader& reader) {
        for (ui32 sizeId = 0; sizeId < 4; ++sizeId) {
            for (ui32 matrixId = 0; matrixId < 6; matrixId += (sizeId == 3) ? 3 : 1) {
                // = scaling_list_pred_mode_flag[sizeId][matrixId]
                const bool scaling_list_pred_mode_flag = reader.ReadBit();

                if (!scaling_list_pred_mode_flag) {
                    ReadUEV(reader); // = scaling_list_pred_matrix_id_delta[sizeId][matrixId]
                } else {
                    const ui32 coefNum = Min(64, 1 << (4 + (sizeId << 1)));

                    if (sizeId > 1) {
                        ReadSEV(reader); // = scaling_list_dc_coef_minus8[sizeId - 2][matrixId]
                    }

                    for (ui32 i = 0; i < coefNum; ++i) {
                        ReadSEV(reader); // = scaling_list_delta_coef
                    }
                }
            }
        }
    }

    // sub_layer_hrd_parameters( i )
    static void SkipSubLayerHrdParametersHevc(TEPDBitReader& reader, const ui32 cpb_cnt_minus1, const ui32 sub_pic_hrd_params_present_flag) {
        for (ui32 i = 0; i <= cpb_cnt_minus1; ++i) {
            ReadUEV(reader); // = bit_rate_value_minus1[i]
            ReadUEV(reader); // = cpb_size_value_minus1[i]

            if (sub_pic_hrd_params_present_flag) {
                ReadUEV(reader); // = cpb_size_du_value_minus1[i]
                ReadUEV(reader); // = bit_rate_du_value_minus1[i]
            }

            reader.ReadBit(); // = cbr_flag[i]
        }
    }

    // hrd_parameters( commonInfPresentFlag, maxNumSubLayersMinus1 )
    static void SkipHrdParametersHevc(TEPDBitReader& reader, const bool commonInfPresentFlag, const ui32 maxNumSubLayersMinus1) {
        bool sub_pic_hrd_params_present_flag = false;
        bool nal_hrd_parameters_present_flag = false;
        bool vcl_hrd_parameters_present_flag = false;

        if (commonInfPresentFlag) {
            nal_hrd_parameters_present_flag = reader.ReadBit();
            vcl_hrd_parameters_present_flag = reader.ReadBit();

            if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
                sub_pic_hrd_params_present_flag = reader.ReadBit();
                if (sub_pic_hrd_params_present_flag) {
                    reader.Read<ui32, 8>(); // = tick_divisor_minus2
                    reader.Read<ui32, 5>(); // = du_cpb_removal_delay_increment_length_minus1
                    reader.ReadBit();       // = sub_pic_cpb_params_in_pic_timing_sei_flag
                    reader.Read<ui32, 5>(); // = dpb_output_delay_du_length_minus1
                }

                reader.Read<ui32, 4>(); // = bit_rate_scale
                reader.Read<ui32, 4>(); // = cpb_size_scale

                if (sub_pic_hrd_params_present_flag) {
                    reader.Read<ui32, 4>(); // = cpb_size_du_scale
                }

                reader.Read<ui32, 5>(); // = initial_cpb_removal_delay_length_minus1
                reader.Read<ui32, 5>(); // = au_cpb_removal_delay_length_minus1
                reader.Read<ui32, 5>(); // = dpb_output_delay_length_minus1
            }
        }

        for (ui32 i = 0; i <= maxNumSubLayersMinus1; ++i) {
            const bool fixed_pic_rate_general_flag = reader.ReadBit();

            bool fixed_pic_rate_within_cvs_flag = true;
            if (!fixed_pic_rate_general_flag) {
                fixed_pic_rate_within_cvs_flag = reader.ReadBit();
            }

            bool low_delay_hrd_flag = 0;
            if (fixed_pic_rate_within_cvs_flag) {
                ReadUEV(reader); // = elemental_duration_in_tc_minus1[i]
            } else {
                low_delay_hrd_flag = reader.ReadBit();
            }

            ui32 cpb_cnt_minus1 = 0;
            if (!low_delay_hrd_flag) {
                cpb_cnt_minus1 = ReadUEV(reader);
            }

            if (nal_hrd_parameters_present_flag) {
                SkipSubLayerHrdParametersHevc(reader, cpb_cnt_minus1, sub_pic_hrd_params_present_flag);
            }

            if (vcl_hrd_parameters_present_flag) {
                SkipSubLayerHrdParametersHevc(reader, cpb_cnt_minus1, sub_pic_hrd_params_present_flag);
            }
        }
    }

    // vui_parameters( )
    static void SkipVuiParametersHevc(TEPDBitReader& reader, const THevcSpsParsed& sps) {
        if (reader.ReadBit()) { // = aspect_ratio_info_present_flag
            const ui32 aspect_ratio_idc = reader.Read<ui32, 8>();
            if (aspect_ratio_idc == 255) { // = EXTENDED_SAR == 255
                reader.Read<ui32, 16>();   // = sar_width
                reader.Read<ui32, 16>();   // = sar_height
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
                reader.Read<ui32, 8>(); // = matrix_coeffs
            }
        }

        if (reader.ReadBit()) { // = chroma_loc_info_present_flag
            ReadUEV(reader);    // = chroma_sample_loc_type_top_field
            ReadUEV(reader);    // = chroma_sample_loc_type_bottom_field
        }

        reader.ReadBit(); // = neutral_chroma_indication_flag
        reader.ReadBit(); // = field_seq_flag
        reader.ReadBit(); // = frame_field_info_present_flag

        if (reader.ReadBit()) { // = default_display_window_flag
            ReadUEV(reader);    // = def_disp_win_left_offset
            ReadUEV(reader);    // = def_disp_win_right_offset
            ReadUEV(reader);    // = def_disp_win_top_offset
            ReadUEV(reader);    // = def_disp_win_bottom_offset
        }

        if (reader.ReadBit()) {      // = vui_timing_info_present_flag
            reader.Read<ui32, 32>(); // = vui_num_units_in_tick
            reader.Read<ui32, 32>(); // = vui_time_scale

            if (reader.ReadBit()) { // = vui_poc_proportional_to_timing_flag
                ReadUEV(reader);    // = vui_num_ticks_poc_diff_one_minus1
            }

            if (reader.ReadBit()) { // = vui_hrd_parameters_present_flag
                SkipHrdParametersHevc(reader, /*commonInfPresentFlag = */ true, sps.sps_max_sub_layers_minus1);
            }
        }

        if (reader.ReadBit()) { // = bitstream_restriction_flag
            reader.ReadBit();   // = tiles_fixed_structure_flag
            reader.ReadBit();   // = motion_vectors_over_pic_boundaries_flag
            reader.ReadBit();   // = restricted_ref_pic_lists_flag
            ReadUEV(reader);    // = min_spatial_segmentation_idc
            ReadUEV(reader);    // = max_bytes_per_pic_denom
            ReadUEV(reader);    // = max_bits_per_min_cu_denom
            ReadUEV(reader);    // = log2_max_mv_length_horizontal
            ReadUEV(reader);    // = log2_max_mv_length_vertical
        }
    }

    // Sequence parameter set RBSP syntax
    // seq_parameter_set_rbsp( )
    THevcSpsParsed ParseHevcSps(TArrayRef<const ui8> spsData) {
        TEPDBitReader reader(spsData.data(), spsData.size());

        reader.ReadBit(); // = forbidden_zero_bit
        const EHevcNalType nalType = (EHevcNalType)reader.Read<ui32, 6>();
        reader.Read<ui32, 6>(); // = nuh_layer_id
        reader.Read<ui32, 3>(); // = nuh_temporal_id_plus1

        Y_ENSURE(nalType == EHevcNalType::SPS_NUT);

        THevcSpsParsed sps;

        reader.Read<ui32, 4>(); // = sps_video_parameter_set_id
        sps.sps_max_sub_layers_minus1 = reader.Read<ui32, 3>();
        reader.ReadBit(); // = sps_temporal_id_nesting_flag

        SkipProfileTierLevelHevc(reader, /*profilePresentFlag = */ true, sps.sps_max_sub_layers_minus1);

        sps.SpsID = ReadUEV(reader);

        sps.chroma_format_idc = ReadUEV(reader);
        if (sps.chroma_format_idc == 3) {
            sps.separate_colour_plane_flag = reader.ReadBit();
        } else {
            sps.separate_colour_plane_flag = 0;
        }

        sps.pic_width_in_luma_samples = ReadUEV(reader);
        sps.pic_height_in_luma_samples = ReadUEV(reader);

        if (reader.ReadBit()) { // = conformance_window_flag
            ReadUEV(reader);    // = conf_win_left_offset
            ReadUEV(reader);    // = conf_win_right_offset
            ReadUEV(reader);    // = conf_win_top_offset
            ReadUEV(reader);    // = conf_win_bottom_offset
        }

        sps.bit_depth_luma = ReadUEV(reader) + 8;
        sps.bit_depth_chroma = ReadUEV(reader) + 8;
        sps.log2_max_pic_order_cnt_lsb = ReadUEV(reader) + 4;

        const bool sps_sub_layer_ordering_info_present_flag = reader.ReadBit();
        for (ui32 i = (sps_sub_layer_ordering_info_present_flag ? 0 : sps.sps_max_sub_layers_minus1); i <= sps.sps_max_sub_layers_minus1; ++i) {
            ReadUEV(reader); // = sps_max_dec_pic_buffering_minus1[i]
            ReadUEV(reader); // = sps_max_num_reorder_pics[i]
            ReadUEV(reader); // = sps_max_latency_increase_plus1[i]
        }

        sps.log2_min_luma_coding_block_size = ReadUEV(reader) + 3;
        sps.log2_diff_max_min_luma_coding_block_size = ReadUEV(reader);
        ReadUEV(reader); // = log2_min_luma_transform_block_size_minus2
        ReadUEV(reader); // = log2_diff_max_min_luma_transform_block_size
        ReadUEV(reader); // = max_transform_hierarchy_depth_inter
        ReadUEV(reader); // = max_transform_hierarchy_depth_intra

        if (reader.ReadBit()) {     // = scaling_list_enabled_flag
            if (reader.ReadBit()) { // = sps_scaling_list_data_present_flag
                SkipScalingListDataHevc(reader);
            }
        }

        reader.ReadBit(); // = amp_enabled_flag
        sps.sample_adaptive_offset_enabled_flag = reader.ReadBit();

        if (reader.ReadBit()) {     // = pcm_enabled_flag
            reader.Read<ui32, 4>(); //  = pcm_sample_bit_depth_luma_minus1
            reader.Read<ui32, 4>(); // = pcm_sample_bit_depth_chroma_minus1
            ReadUEV(reader);        // = log2_min_pcm_luma_coding_block_size_minus3
            ReadUEV(reader);        // = log2_diff_max_min_pcm_luma_coding_block_size
            reader.ReadBit();       // = pcm_loop_filter_disabled_flag
        }

        sps.num_short_term_ref_pic_sets = ReadUEV(reader);
        Y_ENSURE(sps.num_short_term_ref_pic_sets <= 64);

        sps.st_rps.reserve(sps.num_short_term_ref_pic_sets);
        for (ui32 i = 0; i < sps.num_short_term_ref_pic_sets; ++i) {
            sps.st_rps.push_back(ParseShortTermRefPicSetHevc(reader, sps, i));
        }

        sps.long_term_ref_pics_present_flag = reader.ReadBit();
        if (sps.long_term_ref_pics_present_flag) {
            sps.num_long_term_ref_pics_sps = ReadUEV(reader);
            Y_ENSURE(sps.num_long_term_ref_pics_sps <= 32);

            sps.used_by_curr_pic_lt_sps_flags = 0;
            for (ui32 i = 0; i < sps.num_long_term_ref_pics_sps; ++i) {
                reader.Read<ui32>(sps.log2_max_pic_order_cnt_lsb);            // = lt_ref_pic_poc_lsb_sps[i]
                sps.used_by_curr_pic_lt_sps_flags |= (reader.ReadBit() << i); // store all used_by_curr_pic_lt_sps_flag[i] in single ui32
            }
        }

        sps.sps_temporal_mvp_enabled_flag = reader.ReadBit();
        reader.ReadBit(); // = strong_intra_smoothing_enabled_flag

        if (reader.ReadBit()) { // = vui_parameters_present_flag
            SkipVuiParametersHevc(reader, sps);
        }

        if (reader.ReadBit()) { // = sps_extension_present_flag
            const bool sps_range_extension_flag = reader.ReadBit();
            const bool sps_multilayer_extension_flag = reader.ReadBit();
            const bool sps_3d_extension_flag = reader.ReadBit();
            const bool sps_scc_extension_flag = reader.ReadBit();
            const ui32 sps_extension_4bits = reader.Read<ui32, 4>();

            if (sps_range_extension_flag) {
                SkipSpsRangeExtensionHevc(reader);
            }
            if (sps_multilayer_extension_flag) {
                SkipSpsMultilayerExtensionHevc(reader);
            }
            if (sps_3d_extension_flag) {
                SkipSps3dExtensionHevc(reader);
            }
            if (sps_scc_extension_flag) {
                ParseSpsSccExtensionHevc(reader, sps);
            }
            if (sps_extension_4bits) {
                return sps;
            }
        }

        Y_ENSURE(CheckByteAligmentBits(reader) && reader.GetPosition() == spsData.size());

        return sps;
    }

    // pps_range_extension( )
    static void ParsePpsRangeExtensionHevc(TEPDBitReader& reader, THevcPpsParsed& pps, const bool transform_skip_enabled_flag) {
        if (transform_skip_enabled_flag) {
            ReadUEV(reader); // = log2_max_transform_skip_block_size_minus2
        }

        reader.ReadBit(); // = cross_component_prediction_enabled_flag

        pps.chroma_qp_offset_list_enabled_flag = reader.ReadBit();
        if (pps.chroma_qp_offset_list_enabled_flag) {
            ReadUEV(reader); // = diff_cu_chroma_qp_offset_depth
            const ui32 chroma_qp_offset_list_len_minus1 = ReadUEV(reader);
            for (ui32 i = 0; i <= chroma_qp_offset_list_len_minus1; ++i) {
                ReadSEV(reader); // = cb_qp_offset_list[i]
                ReadSEV(reader); // = cr_qp_offset_list[i]
            }
        }
        ReadUEV(reader); // = log2_sao_offset_scale_luma
        ReadUEV(reader); // = log2_sao_offset_scale_chroma
    }

    // pps_scc_extension( )
    static void ParsePpsSccExtensionHevc(TEPDBitReader& reader, THevcPpsParsed& pps) {
        pps.pps_curr_pic_ref_enabled_flag = reader.ReadBit();

        if (reader.ReadBit()) { // = residual_adaptive_colour_transform_enabled_flag
            pps.pps_slice_act_qp_offsets_present_flag = reader.ReadBit();
            ReadSEV(reader); // = pps_act_y_qp_offset_plus5
            ReadSEV(reader); // = pps_act_cb_qp_offset_plus5
            ReadSEV(reader); // = pps_act_cr_qp_offset_plus3
        }

        if (reader.ReadBit()) { // = pps_palette_predictor_initializers_present_flag
            const ui32 pps_num_palette_predictor_initializers = ReadUEV(reader);
            if (pps_num_palette_predictor_initializers > 0) {
                ui32 chroma_bit_depth_entry = 0; // will no be used if !monochrome_palette_flag

                const bool monochrome_palette_flag = reader.ReadBit();

                const ui32 luma_bit_depth_entry = ReadUEV(reader) + 8;

                if (!monochrome_palette_flag) {
                    chroma_bit_depth_entry = ReadUEV(reader) + 8;
                }

                const ui32 numComps = monochrome_palette_flag ? 1 : 3;

                for (ui32 comp = 0; comp < numComps; ++comp) {
                    for (ui32 i = 0; i < pps_num_palette_predictor_initializers; ++i) {
                        reader.Read<ui32>(comp == 0 ? luma_bit_depth_entry : chroma_bit_depth_entry); // = pps_palette_predictor_initializer[comp][i]
                    }
                }
            }
        }
    }

    // colour_mapping_octants( inpDepth, idxY, idxCb, idxCr, inpLength )
    static void SkipColourMappingOctantsHevc(
        TEPDBitReader& reader,
        const ui32 cm_octant_depth,
        const ui32 partNumY,
        const ui32 cmResLSBits,
        const ui32 inpDepth,
        const ui32 idxY,
        const ui32 idxCb,
        const ui32 idxCr,
        const ui32 inpLength) {
        bool split_octant_flag = false;
        if (inpDepth < cm_octant_depth) {
            split_octant_flag = reader.ReadBit();
        }

        if (split_octant_flag) {
            for (ui32 k = 0; k < 2; ++k) {
                for (ui32 m = 0; m < 2; ++m) {
                    for (ui32 n = 0; n < 2; ++n) {
                        SkipColourMappingOctantsHevc(
                            reader,
                            /* cm_octant_depth = */ cm_octant_depth,
                            /* partNumY        = */ partNumY,
                            /* cmResLSBits     = */ cmResLSBits,
                            /* inpDepth        = */ inpDepth + 1,
                            /* idxY            = */ idxY + partNumY * k * inpLength / 2,
                            /* idxCb           = */ idxCb + m * inpLength / 2,
                            /* idxCr           = */ idxCr + n * inpLength / 2,
                            /* inpLength       = */ inpLength / 2);
                    }
                }
            }
        } else {
            for (ui32 i = 0; i < partNumY; ++i) {
                // idxShiftY = idxY + ( i << ( cm_octant_depth - inpDepth ) )

                for (ui32 j = 0; j < 4; ++j) {
                    if (reader.ReadBit()) { // = coded_res_flag[ idxShiftY ][ idxCb ][ idxCr ][ j ]
                        for (ui32 c = 0; c < 3; ++c) {
                            const ui32 res_coeff_q = ReadUEV(reader);                // = res_coeff_q[ idxShiftY ][ idxCb ][ idxCr ][ j ][ c ]
                            const ui32 res_coeff_r = reader.Read<ui32>(cmResLSBits); // = res_coeff_r[ idxShiftY ][ idxCb ][ idxCr ][ j ][ c ]

                            if (res_coeff_q || res_coeff_r) {
                                reader.ReadBit(); // = res_coeff_s[ idxShiftY ][ idxCb ][ idxCr ][ j ][ c ]
                            }
                        }
                    }
                }
            }
        }
    }

    // colour_mapping_table( )
    static void SkipColourMappingTableHevc(TEPDBitReader& reader) {
        const ui32 num_cm_ref_layers_minus1 = ReadUEV(reader);
        for (ui32 i = 0; i <= num_cm_ref_layers_minus1; ++i) {
            reader.Read<ui32, 6>(); // = cm_ref_layer_id[i]
        }

        const ui32 cm_octant_depth = reader.Read<ui32, 2>();
        const ui32 cm_y_part_num_log2 = reader.Read<ui32, 2>();

        const i32 luma_bit_depth_cm_input_minus8 = ReadUEV(reader);
        ReadUEV(reader); // = chroma_bit_depth_cm_input_minus8
        const i32 luma_bit_depth_cm_output_minus8 = ReadUEV(reader);
        ReadUEV(reader); // chroma_bit_depth_cm_output_minus8

        const i32 cm_res_quant_bits = reader.Read<ui32, 2>();
        const i32 cm_delta_flc_bits_minus1 = reader.Read<ui32, 2>();

        const i32 bitDepthCmInputY = 8 + luma_bit_depth_cm_input_minus8;
        const i32 bitDepthCmOutputY = 8 + luma_bit_depth_cm_output_minus8;

        const ui32 cmResLSBits = Max(0, (10 + bitDepthCmInputY - bitDepthCmOutputY - cm_res_quant_bits - (cm_delta_flc_bits_minus1 + 1)));

        if (cm_octant_depth == 1) {
            ReadSEV(reader); // = cm_adapt_threshold_u_delta
            ReadSEV(reader); // = cm_adapt_threshold_v_delta
        }

        SkipColourMappingOctantsHevc(
            reader,
            /* cm_octant_depth = */ cm_octant_depth,
            /* partNumY        = */ 1 << cm_y_part_num_log2,
            /* cmResLSBits     = */ cmResLSBits,
            /* inpDepth        = */ 0,
            /* idxY            = */ 0,
            /* idxCb           = */ 0,
            /* idxCr           = */ 0,
            /* inpLength       = */ 1 << cm_octant_depth);
    }

    // delta_dlt( i )
    static void SkipDeltaDltHevc(TEPDBitReader& reader, const ui32 pps_bit_depth_for_depth_layers) {
        const ui32 num_val_delta_dlt = reader.Read<ui32>(pps_bit_depth_for_depth_layers);
        if (num_val_delta_dlt > 0) {
            ui32 max_diff = 0;
            if (num_val_delta_dlt > 1) {
                max_diff = reader.Read<ui32>(pps_bit_depth_for_depth_layers);
            }

            ui32 min_diff = max_diff;
            if (num_val_delta_dlt > 2 && max_diff > 0) {
                min_diff = reader.Read<ui32>(CeilLog2(max_diff + 1)) + 1;
            }

            reader.Read<ui32>(pps_bit_depth_for_depth_layers); // = delta_dlt_val0

            if (max_diff > min_diff) {
                for (ui32 k = 1; k < num_val_delta_dlt; ++k) {
                    reader.Read<ui32>(CeilLog2(max_diff - min_diff + 1)); // = delta_val_diff_minus_min[ k ]
                }
            }
        }
    }

    // pps_3d_extension( ) /* specified in Annex I */
    void SkipPps3dExtensionHevc(TEPDBitReader& reader) {
        if (reader.ReadBit()) { // = dlts_present_flag
            const ui32 pps_depth_layers_minus1 = reader.Read<ui32, 6>();
            const ui32 pps_bit_depth_for_depth_layers = reader.Read<ui32, 4>() + 8;

            for (ui32 i = 0; i <= pps_depth_layers_minus1; ++i) {
                if (reader.ReadBit()) { // = dlt_flag[ i ]
                    bool dlt_val_flags_present_flag = false;
                    const bool dlt_pred_flag = reader.ReadBit(); // = dlt_pred_flag[ i ]
                    if (!dlt_pred_flag) {
                        dlt_val_flags_present_flag = reader.ReadBit(); // = dlt_val_flags_present_flag [ i ]
                    }

                    if (dlt_val_flags_present_flag) {
                        const ui32 depthMaxValue = (1 << (pps_bit_depth_for_depth_layers)) - 1;
                        for (ui32 j = 0; j <= depthMaxValue; ++j) {
                            reader.ReadBit(); // = dlt_value_flag[ i ][ j ]
                        }
                    } else {
                        // delta_dlt( i )
                        SkipDeltaDltHevc(reader, pps_bit_depth_for_depth_layers);
                    }
                }
            }
        }
    }

    // pps_multilayer_extension( ) /* specified in Annex F */
    static void SkipPpsMultilayerExtensionHevc(TEPDBitReader& reader) {
        reader.ReadBit(); // = poc_reset_info_present_flag

        if (reader.ReadBit()) {     // = pps_infer_scaling_list_flag
            reader.Read<ui32, 6>(); // = pps_scaling_list_ref_layer_id
        }

        const ui32 num_ref_loc_offsets = ReadUEV(reader);
        for (ui32 i = 0; i < num_ref_loc_offsets; ++i) {
            reader.Read<ui32, 6>(); // = ref_loc_offset_layer_id[i]

            if (reader.ReadBit()) { // = scaled_ref_layer_offset_present_flag
                ReadSEV(reader);    // = scaled_ref_layer_left_offset[ref_loc_offset_layer_id[i]]
                ReadSEV(reader);    // = scaled_ref_layer_top_offset[ref_loc_offset_layer_id[i]]
                ReadSEV(reader);    // = scaled_ref_layer_right_offset[ref_loc_offset_layer_id[i]]
                ReadSEV(reader);    // = scaled_ref_layer_bottom_offset[ref_loc_offset_layer_id[i]]
            }

            if (reader.ReadBit()) { // = ref_region_offset_present_flag
                ReadSEV(reader);    // = ref_region_left_offset[ref_loc_offset_layer_id[i]]
                ReadSEV(reader);    // = ref_region_top_offset[ref_loc_offset_layer_id[i]]
                ReadSEV(reader);    // = ref_region_right_offset[ref_loc_offset_layer_id[i]]
                ReadSEV(reader);    // = ref_region_bottom_offset[ref_loc_offset_layer_id[i]]
            }

            if (reader.ReadBit()) { // = resample_phase_set_present_flag
                ReadUEV(reader);    // = phase_hor_luma[ref_loc_offset_layer_id[i]]
                ReadUEV(reader);    // = phase_ver_luma[ref_loc_offset_layer_id[i]]
                ReadUEV(reader);    // = phase_hor_chroma_plus8[ref_loc_offset_layer_id[i]]
                ReadUEV(reader);    // = phase_ver_chroma_plus8[ref_loc_offset_layer_id[i]]
            }
        }

        if (reader.ReadBit()) { // = colour_mapping_enabled_flag
            // colour_mapping_table( )
            SkipColourMappingTableHevc(reader);
        }
    }

    // Picture parameter set RBSP syntax
    // pic_parameter_set_rbsp( )
    THevcPpsParsed ParseHevcPps(TArrayRef<const ui8> ppsData) {
        TEPDBitReader reader(ppsData.data(), ppsData.size());

        reader.ReadBit(); // = forbidden_zero_bit
        const EHevcNalType nalType = (EHevcNalType)reader.Read<ui32, 6>();
        reader.Read<ui32, 6>(); // = nuh_layer_id
        reader.Read<ui32, 3>(); // = nuh_temporal_id_plus1

        Y_ENSURE(nalType == EHevcNalType::PPS_NUT);

        THevcPpsParsed pps;

        pps.PpsID = ReadUEV(reader);
        pps.SpsID = ReadUEV(reader);

        pps.dependent_slice_segments_enabled_flag = reader.ReadBit();
        pps.output_flag_present_flag = reader.ReadBit();
        pps.num_extra_slice_header_bits = reader.Read<ui32, 3>();

        reader.ReadBit(); // = sign_data_hiding_enabled_flag

        pps.cabac_init_present_flag = reader.ReadBit();

        pps.num_ref_idx[0] = ReadUEV(reader) + 1;
        pps.num_ref_idx[1] = ReadUEV(reader) + 1;

        ReadSEV(reader);  // = init_qp_minus26
        reader.ReadBit(); // = constrained_intra_pred_flag

        const bool transform_skip_enabled_flag = reader.ReadBit();

        if (reader.ReadBit()) { // = cu_qp_delta_enabled_flag
            ReadUEV(reader);    // = diff_cu_qp_delta_depth
        }

        ReadSEV(reader); // = pps_cb_qp_offset
        ReadSEV(reader); // = pps_cr_qp_offset

        pps.pps_slice_chroma_qp_offsets_present_flag = reader.ReadBit();
        pps.weighted_pred_flag = reader.ReadBit();
        pps.weighted_bipred_flag = reader.ReadBit();
        reader.ReadBit(); // = transquant_bypass_enabled_flag
        pps.tiles_enabled_flag = reader.ReadBit();
        pps.entropy_coding_sync_enabled_flag = reader.ReadBit();

        if (pps.tiles_enabled_flag) {
            const ui32 num_tile_columns_minus1 = ReadUEV(reader);
            const ui32 num_tile_rows_minus1 = ReadUEV(reader);

            const bool uniform_spacing_flag = reader.ReadBit();
            if (!uniform_spacing_flag) {
                for (ui32 i = 0; i < num_tile_columns_minus1; ++i) {
                    ReadUEV(reader); // = column_width_minus1[i]
                }
                for (ui32 i = 0; i < num_tile_rows_minus1; ++i) {
                    ReadUEV(reader); // = row_height_minus1[i]
                }
            }
            reader.ReadBit(); // = loop_filter_across_tiles_enabled_flag
        }

        pps.pps_loop_filter_across_slices_enabled_flag = reader.ReadBit();

        if (reader.ReadBit()) { // = deblocking_filter_control_present_flag
            pps.deblocking_filter_override_enabled_flag = reader.ReadBit();
            pps.pps_deblocking_filter_disabled_flag = reader.ReadBit();
            if (!pps.pps_deblocking_filter_disabled_flag) {
                ReadSEV(reader); // = pps_beta_offset_div2
                ReadSEV(reader); // = pps_tc_offset_div2
            }
        }

        if (reader.ReadBit()) { // = pps_scaling_list_data_present_flag
            SkipScalingListDataHevc(reader);
        }

        pps.lists_modification_present_flag = reader.ReadBit();
        ReadUEV(reader); // = log2_parallel_merge_level_minus2
        pps.slice_segment_header_extension_present_flag = reader.ReadBit();

        if (reader.ReadBit()) { // = pps_extension_present_flag
            const bool pps_range_extension_flag = reader.ReadBit();
            const bool pps_multilayer_extension_flag = reader.ReadBit();
            const bool pps_3d_extension_flag = reader.ReadBit();
            const bool pps_scc_extension_flag = reader.ReadBit();
            const bool pps_extension_4bits = reader.Read<ui32, 4>();

            if (pps_range_extension_flag) {
                ParsePpsRangeExtensionHevc(reader, pps, transform_skip_enabled_flag);
            }

            if (pps_multilayer_extension_flag) {
                SkipPpsMultilayerExtensionHevc(reader);
            }

            if (pps_3d_extension_flag) {
                SkipPps3dExtensionHevc(reader);
            }

            if (pps_scc_extension_flag) {
                ParsePpsSccExtensionHevc(reader, pps);
            }

            if (pps_extension_4bits) {
                return pps;
            }
        }

        Y_ENSURE(CheckByteAligmentBits(reader) && reader.GetPosition() == ppsData.size());

        return pps;
    }

    // General slice segment header syntax
    //  slice_segment_header( )
    static void SkipSliceHeaderHevc(
        TEPDBitReader& reader,
        const EHevcNalType nalType,
        const TVector<THevcSpsParsed>& spsVector,
        const TVector<THevcPpsParsed>& ppsVector) {
        Y_ENSURE(!spsVector.empty() && !ppsVector.empty());

        const bool first_slice_segment_in_pic_flag = reader.ReadBit();
        if (nalType >= EHevcNalType::BLA_W_LP && nalType <= EHevcNalType::RSV_IRAP_VCL23) {
            reader.ReadBit(); // = no_output_of_prior_pics_flag
        }

        const ui32 ppsID = ReadUEV(reader);

        THevcPpsParsed const* const ppsPtr = FindPps(ppsVector, ppsID);
        Y_ENSURE(ppsPtr, "pps id " << ppsID << " not found at SkipSliceHeaderHevc");
        const THevcPpsParsed& pps = *ppsPtr;

        THevcSpsParsed const* const spsPtr = FindSps(spsVector, pps.SpsID);
        Y_ENSURE(spsPtr, "sps id " << pps.SpsID << " not found at SkipSliceHeaderHevc");
        const THevcSpsParsed& sps = *spsPtr;

        const int chromaArrayType = sps.separate_colour_plane_flag ? 0 : sps.chroma_format_idc;

        bool dependent_slice_segment_flag = false;
        if (!first_slice_segment_in_pic_flag) {
            if (pps.dependent_slice_segments_enabled_flag) {
                dependent_slice_segment_flag = reader.ReadBit();
            }

            const ui32 ctbSizeY = 1 << (sps.log2_min_luma_coding_block_size + sps.log2_diff_max_min_luma_coding_block_size);
            const ui32 picWidthInCtbsY = DivCeil(sps.pic_width_in_luma_samples, ctbSizeY);
            const ui32 picHeightInCtbsY = DivCeil(sps.pic_height_in_luma_samples, ctbSizeY);
            const ui32 picSizeInCtbsY = picWidthInCtbsY * picHeightInCtbsY;

            reader.Read<ui32>(CeilLog2(picSizeInCtbsY)); // = slice_segment_address
        }

        if (!dependent_slice_segment_flag) {
            for (ui32 i = 0; i < pps.num_extra_slice_header_bits; ++i) {
                reader.ReadBit(); // = slice_reserved_flag[i]
            }

            const EHevcSlice slice_type = (EHevcSlice)ReadUEV(reader);

            if (pps.output_flag_present_flag) {
                reader.ReadBit(); // = pic_output_flag
            }
            if (sps.separate_colour_plane_flag) {
                reader.Read<ui32, 2>(); // = colour_plane_id
            }

            ui32 lt_used_by_curr_pic_sum = 0; // long term part of numPicTotalCurr
            bool slice_temporal_mvp_enabled_flag = false;

            TMaybe<THevcShortTermRPS> st_rps_tmp;
            THevcShortTermRPS const* st_rps = nullptr;

            if (nalType != EHevcNalType::IDR_W_RADL && nalType != EHevcNalType::IDR_N_LP) {
                reader.Read<ui32>(sps.log2_max_pic_order_cnt_lsb); // = slice_pic_order_cnt_lsb

                const bool short_term_ref_pic_set_sps_flag = reader.ReadBit();
                if (!short_term_ref_pic_set_sps_flag) {
                    st_rps_tmp = ParseShortTermRefPicSetHevc(reader, sps, sps.num_short_term_ref_pic_sets);
                    st_rps = &*st_rps_tmp;
                } else {
                    ui32 short_term_ref_pic_set_idx = 0;
                    if (sps.num_short_term_ref_pic_sets > 1) {
                        short_term_ref_pic_set_idx = reader.Read<ui32>(CeilLog2(sps.num_short_term_ref_pic_sets));
                    } else {
                        Y_ENSURE(sps.num_short_term_ref_pic_sets > 0);
                    }

                    Y_ENSURE(short_term_ref_pic_set_idx < sps.num_short_term_ref_pic_sets);

                    st_rps = &sps.st_rps[short_term_ref_pic_set_idx];
                }

                if (sps.long_term_ref_pics_present_flag) {
                    ui32 num_long_term_sps = 0;
                    if (sps.num_long_term_ref_pics_sps > 0) {
                        num_long_term_sps = ReadUEV(reader);
                    }
                    const ui32 num_long_term_pics = ReadUEV(reader);
                    for (ui32 i = 0; i < num_long_term_sps + num_long_term_pics; ++i) {
                        if (i < num_long_term_sps) {
                            if (sps.num_long_term_ref_pics_sps > 1) {
                                const ui32 lt_idx_sps_i = reader.Read<ui32>(CeilLog2(sps.num_long_term_ref_pics_sps)); // = lt_idx_sps[i]
                                lt_used_by_curr_pic_sum += (sps.used_by_curr_pic_lt_sps_flags >> lt_idx_sps_i) & 1;
                            }
                        } else {
                            reader.Read<ui32>(sps.log2_max_pic_order_cnt_lsb); // = poc_lsb_lt[i]
                            lt_used_by_curr_pic_sum += reader.ReadBit();       // = used_by_curr_pic_lt_flag[i]
                        }

                        if (reader.ReadBit()) { // = delta_poc_msb_present_flag
                            ReadSEV(reader);    // = delta_poc_msb_cycle_lt[i]
                        }
                    }
                }

                if (sps.sps_temporal_mvp_enabled_flag) {
                    slice_temporal_mvp_enabled_flag = reader.ReadBit();
                }
            }

            bool slice_sao_luma_flag = false;
            bool slice_sao_chroma_flag = false;

            if (sps.sample_adaptive_offset_enabled_flag) {
                slice_sao_luma_flag = reader.ReadBit();

                if (chromaArrayType != 0) {
                    slice_sao_chroma_flag = reader.ReadBit();
                }
            }

            if (slice_type == EHevcSlice::P || slice_type == EHevcSlice::B) {
                // {num_ref_idx_l0_active_minus1 + 1, num_ref_idx_l1_active_minus1 + 1}
                ui32 num_ref_idx[2] = {pps.num_ref_idx[0], pps.num_ref_idx[1]};

                if (reader.ReadBit()) { // = num_ref_idx_active_override_flag
                    num_ref_idx[0] = ReadUEV(reader) + 1;
                    if (slice_type == EHevcSlice::B) {
                        num_ref_idx[1] = ReadUEV(reader) + 1;
                    }
                }

                if (pps.lists_modification_present_flag) {
                    const ui32 numPicTotalCurr = // TODO: check carefully after parsed SPS and PPS
                        (st_rps ? GetShortTermUsedPicsCountHevc(*st_rps) : 0) +
                        lt_used_by_curr_pic_sum +
                        (pps.pps_curr_pic_ref_enabled_flag ? 1 : 0);

                    if (numPicTotalCurr > 1) {
                        SkipRefPicListsModificationHevc(reader, slice_type, num_ref_idx, numPicTotalCurr);
                    }
                }

                if (slice_type == EHevcSlice::B) {
                    reader.ReadBit(); // = mvd_l1_zero_flag
                }

                if (pps.cabac_init_present_flag) {
                    reader.ReadBit(); // = cabac_init_flag
                }

                if (slice_temporal_mvp_enabled_flag) {
                    bool collocated_from_l0_flag = true;
                    if (slice_type == EHevcSlice::B) {
                        collocated_from_l0_flag = reader.ReadBit();
                    }
                    if ((collocated_from_l0_flag && num_ref_idx[0] > 1) || (!collocated_from_l0_flag && num_ref_idx[1] > 1)) {
                        ReadUEV(reader); // = collocated_ref_idx
                    }
                }

                if ((pps.weighted_pred_flag && slice_type == EHevcSlice::P) || (pps.weighted_bipred_flag && slice_type == EHevcSlice::B)) {
                    SkipPredWeightTableHevc(reader, slice_type, num_ref_idx, chromaArrayType);
                }

                ReadUEV(reader); // = five_minus_max_num_merge_cand

                if (sps.motion_vector_resolution_control_idc == 2) {
                    reader.ReadBit(); // = use_integer_mv_flag
                }
            }

            ReadSEV(reader); // = slice_qp_delta
            if (pps.pps_slice_chroma_qp_offsets_present_flag) {
                ReadSEV(reader); // = slice_cb_qp_offset
                ReadSEV(reader); // = slice_cr_qp_offset
            }

            if (pps.pps_slice_act_qp_offsets_present_flag) {
                ReadSEV(reader); // = slice_act_y_qp_offset
                ReadSEV(reader); // = slice_act_cb_qp_offset
                ReadSEV(reader); // = slice_act_cr_qp_offset
            }

            if (pps.chroma_qp_offset_list_enabled_flag) {
                reader.ReadBit(); // = cu_chroma_qp_offset_enabled_flag
            }

            bool deblocking_filter_override_flag = 0;
            if (pps.deblocking_filter_override_enabled_flag) {
                deblocking_filter_override_flag = reader.ReadBit();
            }

            bool slice_deblocking_filter_disabled_flag = pps.pps_deblocking_filter_disabled_flag;
            if (deblocking_filter_override_flag) {
                slice_deblocking_filter_disabled_flag = reader.ReadBit();
                if (!slice_deblocking_filter_disabled_flag) {
                    ReadSEV(reader); // = slice_beta_offset_div2
                    ReadSEV(reader); // = slice_tc_offset_div2
                }
            }

            if (pps.pps_loop_filter_across_slices_enabled_flag && (slice_sao_luma_flag || slice_sao_chroma_flag || !slice_deblocking_filter_disabled_flag)) {
                reader.ReadBit(); // = slice_loop_filter_across_slices_enabled_flag
            }
        }

        if (pps.tiles_enabled_flag || pps.entropy_coding_sync_enabled_flag) {
            const ui32 num_entry_point_offsets = ReadUEV(reader);
            if (num_entry_point_offsets > 0) {
                const ui32 offset_len_minus1 = ReadUEV(reader);
                for (ui32 i = 0; i < num_entry_point_offsets; ++i) {
                    reader.Read<ui32>(offset_len_minus1 + 1); // = entry_point_offset_minus1[i]
                }
            }
        }

        if (pps.slice_segment_header_extension_present_flag) {
            const ui32 slice_segment_header_extension_length = ReadUEV(reader);
            for (ui32 i = 0; i < slice_segment_header_extension_length; ++i) {
                // at least this extension can be skipped without parsing
                reader.Read<ui8>(8);
            }
        }

        Y_ENSURE(CheckByteAligmentBits(reader));
    }

    TMaybe<ui32> GetSliceDataOffsetHevc(
        ui8 const* const data,
        const size_t dataSize,
        TVector<THevcSpsParsed>& spsVector,
        TVector<THevcPpsParsed>& ppsVector) {
        TEPDBitReader reader(data, dataSize);

        reader.ReadBit(); // = forbidden_zero_bit
        const EHevcNalType nalType = (EHevcNalType)reader.Read<ui32, 6>();
        reader.Read<ui32, 6>(); // = nuh_layer_id
        reader.Read<ui32, 3>(); // = nuh_temporal_id_plus1

        if (nalType < EHevcNalType::VPS_NUT) {
            SkipSliceHeaderHevc(reader, nalType, spsVector, ppsVector);
            return reader.GetPosition();
        } else if (nalType == EHevcNalType::SPS_NUT) {
            THevcSpsParsed sps = ParseHevcSps({data, dataSize});
            if (THevcSpsParsed* tptr = FindSps(spsVector, sps.SpsID)) {
                *tptr = std::move(sps);
            } else {
                spsVector.push_back(std::move(sps));
            }
            return {};
        } else if (nalType == EHevcNalType::PPS_NUT) {
            THevcPpsParsed pps = ParseHevcPps({data, dataSize});
            if (THevcPpsParsed* tptr = FindPps(ppsVector, pps.PpsID)) {
                *tptr = std::move(pps);
            } else {
                ppsVector.push_back(std::move(pps));
            }
            return {};
        } else {
            return {};
        }
    }

}
