IF (NOT AUTOCHECK)

OWNER(g:cloud-asr)

UNION()

FILES(
    text_preprocessor_config.json
    features_extractor_config.json
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
    2726021003 OUT
    text_preprocessor/cloud_subst.conf
    text_preprocessor/label.conf
    text_preprocessor/punc.conf
    text_preprocessor/subst.conf
    text_preprocessor/chunker/abbrev-simple.txt
    text_preprocessor/chunker/chunker.conf
    text_preprocessor/domain_subst/music_subst.txt
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
    text_preprocessor/normalizer/address.expand_address.fst
    text_preprocessor/normalizer/address.expand_city_with_name.fst
    text_preprocessor/normalizer/address.expand_metro.fst
    text_preprocessor/normalizer/case_control.insert_control_marks.fst
    text_preprocessor/normalizer/case_control.remove_control_marks.fst
    text_preprocessor/normalizer/date.convert_date.fst
    text_preprocessor/normalizer/flags.txt
    text_preprocessor/normalizer/hyphen_deletion.hyphen_deletion.fst
    text_preprocessor/normalizer/ip_addr.convert_ip_address.fst
    text_preprocessor/normalizer/math_expressions.convert_abbrs_and_equal.fst
    text_preprocessor/normalizer/math_expressions.convert_math.fst
    text_preprocessor/normalizer/music_domain.music_top_transcriptions.fst
    text_preprocessor/normalizer/names.name_abbreviation.fst
    text_preprocessor/normalizer/number.convert_cardinal.fst
    text_preprocessor/normalizer/number.convert_fraction.fst
    text_preprocessor/normalizer/number.convert_ordinal.fst
    text_preprocessor/normalizer/number.convert_upper_register.fst
    text_preprocessor/normalizer/number.numbers_case_control.fst
    text_preprocessor/normalizer/number.prepare_cardinal.fst
    text_preprocessor/normalizer/number.prepare_ordinal.fst
    text_preprocessor/normalizer/number.use_ordinal_markers.fst
    text_preprocessor/normalizer/roman.convert_roman.fst
    text_preprocessor/normalizer/sequence.txt
    text_preprocessor/normalizer/simple_conversions.camelCase_split.fst
    text_preprocessor/normalizer/simple_conversions.collapse_spaces.fst
    text_preprocessor/normalizer/simple_conversions.convert_digit_sequences.fst
    text_preprocessor/normalizer/simple_conversions.convert_slash_between_numbers.fst
    text_preprocessor/normalizer/simple_conversions.final_spaces_adjustment.fst
    text_preprocessor/normalizer/simple_conversions.glue_punctuation.fst
    text_preprocessor/normalizer/simple_conversions.lower_and_whitespace.fst
    text_preprocessor/normalizer/simple_conversions.marriage.fst
    text_preprocessor/normalizer/simple_conversions.number_ranges.fst
    text_preprocessor/normalizer/simple_conversions.plus_minus.fst
    text_preprocessor/normalizer/simple_conversions.replace_sharp.fst
    text_preprocessor/normalizer/simple_conversions.space_at_start_end.fst
    text_preprocessor/normalizer/simple_conversions.spaces_around_punctuation.fst
    text_preprocessor/normalizer/simple_conversions.split_l2d.fst
    text_preprocessor/normalizer/simple_conversions.strip_accents.fst
    text_preprocessor/normalizer/simple_conversions.substitutions_first_stage.fst
    text_preprocessor/normalizer/simple_conversions.substitutions_second_stage.fst
    text_preprocessor/normalizer/simple_conversions.transform_email.fst
    text_preprocessor/normalizer/simple_conversions.transform_url.fst
    text_preprocessor/normalizer/simple_conversions.unguard.fst
    text_preprocessor/normalizer/simple_conversions.words_to_phrases.fst
    text_preprocessor/normalizer/spec_codes.special_codes.fst
    text_preprocessor/normalizer/sport.expand_score.fst
    text_preprocessor/normalizer/symbols.sym
    text_preprocessor/normalizer/telephone.convert_telephone.fst
    text_preprocessor/normalizer/time_cvt.convert_time.fst
    text_preprocessor/normalizer/units.convert_number_abbr.fst
    text_preprocessor/normalizer/units.convert_size.fst
    text_preprocessor/normalizer/units.convert_units.fst
    text_preprocessor/normalizer/units.convert_units_with_slash.fst
    text_preprocessor/normalizer/units.expand_degrees.fst
    text_preprocessor/pauser/map.json
    text_preprocessor/pauser/weights.npz
)

END()

ENDIF()
