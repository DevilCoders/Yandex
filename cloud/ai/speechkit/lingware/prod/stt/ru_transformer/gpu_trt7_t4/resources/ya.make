OWNER(g:cloud-asr)

UNION()

FILES(
    asr_system_config.json
    features_config.json
    resources_spec.json
    ru/am_config.json
    ru/asr_transformer_config.json
    ru/letters.lst
    ru/nn_normalizer_config.json
)

FROM_SANDBOX(FILE 3233783693 RENAME RESOURCE OUT ru/jasper.trt)
FROM_SANDBOX(FILE 3159435901 RENAME RESOURCE OUT ru/asr_transformer.npz)
FROM_SANDBOX(FILE 3159436150 RENAME RESOURCE OUT ru/transformer_dictionary.txt)

FROM_SANDBOX(FILE 1325875511 RENAME RESOURCE OUT fst_normalizer.tar.gz)
FROM_SANDBOX(FILE 3179031355 RENAME RESOURCE OUT ru/mt_normalizer.tar)

END()
