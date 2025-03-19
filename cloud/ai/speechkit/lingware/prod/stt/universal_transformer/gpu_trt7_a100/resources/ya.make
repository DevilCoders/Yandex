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
    auto/am_config.json
    auto/asr_transformer_config.json
    auto/letters.lst
    multilang/asr_transformer_config.json
    multilang/language_embedding_config.json
)

FROM_SANDBOX(FILE 2848811125 RENAME RESOURCE OUT ru/jasper.trt)
FROM_SANDBOX(FILE 3159435901 RENAME RESOURCE OUT ru/asr_transformer.npz)
FROM_SANDBOX(FILE 3159436150 RENAME RESOURCE OUT ru/transformer_dictionary.txt)

FROM_SANDBOX(FILE 2980365226 RENAME RESOURCE OUT auto/jasper.trt)
FROM_SANDBOX(FILE 2975859283 RENAME RESOURCE OUT auto/asr_transformer.npz)
FROM_SANDBOX(FILE 2975859152 RENAME RESOURCE OUT auto/transformer_dictionary.txt)

FROM_SANDBOX(FILE 3259221599 RENAME RESOURCE OUT multilang/asr_transformer.npz)
FROM_SANDBOX(FILE 3259220796 RENAME RESOURCE OUT multilang/transformer_dictionary.txt)
FROM_SANDBOX(FILE 3259326543 RENAME RESOURCE OUT multilang/language_embedding.npz)

FROM_SANDBOX(FILE 1325875511 RENAME RESOURCE OUT fst_normalizer.tar.gz)
FROM_SANDBOX(FILE 3179031355 RENAME RESOURCE OUT ru/mt_normalizer.tar)

END()
