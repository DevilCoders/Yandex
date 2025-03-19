OWNER(g:cloud-asr)

UNION(normalizer)

FROM_SANDBOX(1720843474
    RENAME normalizer/case_to_features.cvt.fst OUT_NOAUTO case_to_features.cvt.fst
    RENAME normalizer/simple_conversions.ignore_marks_near_numbers_cvt.fst OUT_NOAUTO simple_conversions.ignore_marks_near_numbers_cvt.fst
    RENAME normalizer/convert_numbers.cvt.fst OUT_NOAUTO convert_numbers.cvt.fst
    RENAME normalizer/simple_conversions.glue_punctuation_cvt.fst OUT_NOAUTO simple_conversions.glue_punctuation_cvt.fst
    RENAME normalizer/units.cvt.fst OUT_NOAUTO units.cvt.fst
    RENAME normalizer/simple_conversions.split_l2d_cvt.fst OUT_NOAUTO simple_conversions.split_l2d_cvt.fst
    RENAME normalizer/simple_conversions.final_spaces_adjustment.fst OUT_NOAUTO simple_conversions.final_spaces_adjustment.fst
    RENAME normalizer/simple_conversions.replace_unconditionally_cvt.fst OUT_NOAUTO simple_conversions.replace_unconditionally_cvt.fst
    RENAME normalizer/latitude_longitude.cvt.fst OUT_NOAUTO latitude_longitude.cvt.fst
    RENAME normalizer/simple_conversions.replace_punctuation_cvt.fst OUT_NOAUTO simple_conversions.replace_punctuation_cvt.fst
    RENAME normalizer/simple_conversions.lower_and_whitespace_cvt.fst OUT_NOAUTO simple_conversions.lower_and_whitespace_cvt.fst
    RENAME normalizer/date.cvt.fst OUT_NOAUTO date.cvt.fst
    RENAME normalizer/simple_conversions.ranges_cvt.fst OUT_NOAUTO simple_conversions.ranges_cvt.fst
    RENAME normalizer/simple_conversions.remove_space_at_end.fst OUT_NOAUTO simple_conversions.remove_space_at_end.fst
    RENAME normalizer/time_cvt.cvt.fst OUT_NOAUTO time_cvt.cvt.fst
    RENAME normalizer/sequence.txt OUT_NOAUTO sequence.txt
    RENAME normalizer/symbols.sym OUT_NOAUTO symbols.sym
    RENAME normalizer/simple_conversions.replace_short_exclamation.fst OUT_NOAUTO simple_conversions.replace_short_exclamation.fst
    RENAME normalizer/simple_conversions.spaces_around_punctuation.fst OUT_NOAUTO simple_conversions.spaces_around_punctuation.fst
    RENAME normalizer/simple_conversions.collapse_spaces.fst OUT_NOAUTO simple_conversions.collapse_spaces.fst
    RENAME normalizer/simple_conversions.space_at_start.fst OUT_NOAUTO simple_conversions.space_at_start.fst
    RENAME normalizer/flags.txt OUT_NOAUTO flags.txt
)

END()
