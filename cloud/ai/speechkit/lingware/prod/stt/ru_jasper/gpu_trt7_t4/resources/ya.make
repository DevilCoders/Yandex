OWNER(g:cloud-asr)

UNION()

FILES(
    asr_system_config.json
    features_config.json
    resources_spec.json
    ru/am_config.json
    ru/letters.lst
    ru/kenlm_config.json
    ru/beam_search_config.json
)

FROM_SANDBOX(FILE 3233764044 RENAME RESOURCE OUT ru/jasper.trt)
FROM_SANDBOX(FILE 1424298322 RENAME RESOURCE OUT ru/words.lst)
FROM_SANDBOX(FILE 1418279803 RENAME RESOURCE OUT ru/lm)
FROM_SANDBOX(FILE 1325875511 RENAME RESOURCE OUT fst_normalizer.tar.gz)

END()
