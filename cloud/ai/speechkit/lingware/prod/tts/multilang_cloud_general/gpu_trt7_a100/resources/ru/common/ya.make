IF (NOT AUTOCHECK)

OWNER(g:cloud-asr)

UNION()

FILES(
    chunker_config.json
    features_extractor_config.json
    normalizer_config.json
    text_preprocessor_config.json
    vocoder_config.json
)

FROM_SANDBOX(
    2861894501 OUT
    features_extractor/features_extractor.trt
)

FROM_SANDBOX(
    2861889702 OUT
    vocoder/vocoder.trt
)

FROM_SANDBOX(
    3230273336
    RENAME text_preprocessor/chunker/abbrev-simple.txt OUT_NOAUTO chunker/abbrev-simple.txt
    RENAME text_preprocessor/chunker/chunker.conf OUT_NOAUTO chunker/chunker.conf
)

FROM_SANDBOX(
    3230273336
    RENAME text_preprocessor/domain_subst/music_subst.txt OUT_NOAUTO normalizer/domain_subst/music_subst.txt
    RENAME text_preprocessor/normalizer/address.expand_address.fst OUT_NOAUTO normalizer/normalizer/address.expand_address.fst
    RENAME text_preprocessor/normalizer/address.expand_city_with_name.fst OUT_NOAUTO normalizer/normalizer/address.expand_city_with_name.fst
    RENAME text_preprocessor/normalizer/address.expand_metro.fst OUT_NOAUTO normalizer/normalizer/address.expand_metro.fst
    RENAME text_preprocessor/normalizer/case_control.insert_control_marks.fst OUT_NOAUTO normalizer/normalizer/case_control.insert_control_marks.fst
    RENAME text_preprocessor/normalizer/case_control.remove_control_marks.fst OUT_NOAUTO normalizer/normalizer/case_control.remove_control_marks.fst
    RENAME text_preprocessor/normalizer/date.convert_date.fst OUT_NOAUTO normalizer/normalizer/date.convert_date.fst
    RENAME text_preprocessor/normalizer/flags.txt OUT_NOAUTO normalizer/normalizer/flags.txt
    RENAME text_preprocessor/normalizer/hyphen_deletion.hyphen_deletion.fst OUT_NOAUTO normalizer/normalizer/hyphen_deletion.hyphen_deletion.fst
    RENAME text_preprocessor/normalizer/ip_addr.convert_ip_address.fst OUT_NOAUTO normalizer/normalizer/ip_addr.convert_ip_address.fst
    RENAME text_preprocessor/normalizer/math_expressions.convert_abbrs_and_equal.fst OUT_NOAUTO normalizer/normalizer/math_expressions.convert_abbrs_and_equal.fst
    RENAME text_preprocessor/normalizer/math_expressions.convert_math.fst OUT_NOAUTO normalizer/normalizer/math_expressions.convert_math.fst
    RENAME text_preprocessor/normalizer/music_domain.music_top_transcriptions.fst OUT_NOAUTO normalizer/normalizer/music_domain.music_top_transcriptions.fst
    RENAME text_preprocessor/normalizer/names.name_abbreviation.fst OUT_NOAUTO normalizer/normalizer/names.name_abbreviation.fst
    RENAME text_preprocessor/normalizer/number.convert_cardinal.fst OUT_NOAUTO normalizer/normalizer/number.convert_cardinal.fst
    RENAME text_preprocessor/normalizer/number.convert_fraction.fst OUT_NOAUTO normalizer/normalizer/number.convert_fraction.fst
    RENAME text_preprocessor/normalizer/number.convert_ordinal.fst OUT_NOAUTO normalizer/normalizer/number.convert_ordinal.fst
    RENAME text_preprocessor/normalizer/number.convert_upper_register.fst OUT_NOAUTO normalizer/normalizer/number.convert_upper_register.fst
    RENAME text_preprocessor/normalizer/number.numbers_case_control.fst OUT_NOAUTO normalizer/normalizer/number.numbers_case_control.fst
    RENAME text_preprocessor/normalizer/number.prepare_cardinal.fst OUT_NOAUTO normalizer/normalizer/number.prepare_cardinal.fst
    RENAME text_preprocessor/normalizer/number.prepare_ordinal.fst OUT_NOAUTO normalizer/normalizer/number.prepare_ordinal.fst
    RENAME text_preprocessor/normalizer/number.use_ordinal_markers.fst OUT_NOAUTO normalizer/normalizer/number.use_ordinal_markers.fst
    RENAME text_preprocessor/normalizer/roman.convert_roman.fst OUT_NOAUTO normalizer/normalizer/roman.convert_roman.fst
    RENAME text_preprocessor/normalizer/sequence.txt OUT_NOAUTO normalizer/normalizer/sequence.txt
    RENAME text_preprocessor/normalizer/simple_conversions.camelCase_split.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.camelCase_split.fst
    RENAME text_preprocessor/normalizer/simple_conversions.collapse_spaces.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.collapse_spaces.fst
    RENAME text_preprocessor/normalizer/simple_conversions.convert_digit_sequences.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.convert_digit_sequences.fst
    RENAME text_preprocessor/normalizer/simple_conversions.convert_slash_between_numbers.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.convert_slash_between_numbers.fst
    RENAME text_preprocessor/normalizer/simple_conversions.final_spaces_adjustment.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.final_spaces_adjustment.fst
    RENAME text_preprocessor/normalizer/simple_conversions.glue_punctuation.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.glue_punctuation.fst
    RENAME text_preprocessor/normalizer/simple_conversions.lower_and_whitespace.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.lower_and_whitespace.fst
    RENAME text_preprocessor/normalizer/simple_conversions.marriage.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.marriage.fst
    RENAME text_preprocessor/normalizer/simple_conversions.number_ranges.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.number_ranges.fst
    RENAME text_preprocessor/normalizer/simple_conversions.plus_minus.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.plus_minus.fst
    RENAME text_preprocessor/normalizer/simple_conversions.replace_sharp.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.replace_sharp.fst
    RENAME text_preprocessor/normalizer/simple_conversions.space_at_start_end.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.space_at_start_end.fst
    RENAME text_preprocessor/normalizer/simple_conversions.spaces_around_punctuation.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.spaces_around_punctuation.fst
    RENAME text_preprocessor/normalizer/simple_conversions.split_l2d.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.split_l2d.fst
    RENAME text_preprocessor/normalizer/simple_conversions.strip_accents.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.strip_accents.fst
    RENAME text_preprocessor/normalizer/simple_conversions.substitutions_first_stage.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.substitutions_first_stage.fst
    RENAME text_preprocessor/normalizer/simple_conversions.substitutions_second_stage.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.substitutions_second_stage.fst
    RENAME text_preprocessor/normalizer/simple_conversions.transform_email.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.transform_email.fst
    RENAME text_preprocessor/normalizer/simple_conversions.transform_url.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.transform_url.fst
    RENAME text_preprocessor/normalizer/simple_conversions.unguard.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.unguard.fst
    RENAME text_preprocessor/normalizer/simple_conversions.words_to_phrases.fst OUT_NOAUTO normalizer/normalizer/simple_conversions.words_to_phrases.fst
    RENAME text_preprocessor/normalizer/spec_codes.special_codes.fst OUT_NOAUTO normalizer/normalizer/spec_codes.special_codes.fst
    RENAME text_preprocessor/normalizer/sport.expand_score.fst OUT_NOAUTO normalizer/normalizer/sport.expand_score.fst
    RENAME text_preprocessor/normalizer/symbols.sym OUT_NOAUTO normalizer/normalizer/symbols.sym
    RENAME text_preprocessor/normalizer/telephone.convert_telephone.fst OUT_NOAUTO normalizer/normalizer/telephone.convert_telephone.fst
    RENAME text_preprocessor/normalizer/time_cvt.convert_time.fst OUT_NOAUTO normalizer/normalizer/time_cvt.convert_time.fst
    RENAME text_preprocessor/normalizer/units.convert_number_abbr.fst OUT_NOAUTO normalizer/normalizer/units.convert_number_abbr.fst
    RENAME text_preprocessor/normalizer/units.convert_size.fst OUT_NOAUTO normalizer/normalizer/units.convert_size.fst
    RENAME text_preprocessor/normalizer/units.convert_units.fst OUT_NOAUTO normalizer/normalizer/units.convert_units.fst
    RENAME text_preprocessor/normalizer/units.convert_units_with_slash.fst OUT_NOAUTO normalizer/normalizer/units.convert_units_with_slash.fst
    RENAME text_preprocessor/normalizer/units.expand_degrees.fst OUT_NOAUTO normalizer/normalizer/units.expand_degrees.fst    
)

FROM_SANDBOX(
    3230273336 OUT
    text_preprocessor/cloud_subst.conf
    text_preprocessor/label.conf
    text_preprocessor/punc.conf
    text_preprocessor/subst.conf
    text_preprocessor/g2p/config
    text_preprocessor/g2p/cloud_general.dict
    text_preprocessor/g2p/general.dict
    text_preprocessor/g2p/hard_to_soft.map
    text_preprocessor/g2p/lts.map
    text_preprocessor/g2p/lts.re
    text_preprocessor/g2p/prepositions.txt
    text_preprocessor/g2p/ru.auxilary_tags
    text_preprocessor/g2p/ru.grapheme_groups
    text_preprocessor/g2p/ru.lts_phone_groups
    text_preprocessor/g2p/RU.phone
    text_preprocessor/g2p/ru.valid_graphemes
    text_preprocessor/g2p/unvoiced_to_voiced.map
    text_preprocessor/g2p/mt_transcriber/conf/mt_transcribe.json
    text_preprocessor/g2p/mt_transcriber/data/en-ru.translit.txt
    text_preprocessor/g2p/mt_transcriber/data/en.transformer.npz
    text_preprocessor/g2p/mt_transcriber/data/en.voc
    text_preprocessor/g2p/mt_transcriber/data/glue.txt
    text_preprocessor/g2p/mt_transcriber/data/lang_detector_model.arch
    text_preprocessor/g2p/mt_transcriber/data/phonemes.voc
    text_preprocessor/g2p/mt_transcriber/data/ru-en.translit.txt
    text_preprocessor/g2p/mt_transcriber/data/ru.transformer.npz
    text_preprocessor/g2p/mt_transcriber/data/ru.voc
    text_preprocessor/homograph_resolver/bpe.voc
    text_preprocessor/homograph_resolver/bpe2num.voc
    text_preprocessor/homograph_resolver/conf.json
    text_preprocessor/homograph_resolver/map.json
    text_preprocessor/homograph_resolver/transformer.npz
    text_preprocessor/homograph_resolver/weights.npz
    text_preprocessor/pauser/map.json
    text_preprocessor/pauser/weights.npz
)

END()

ENDIF()
