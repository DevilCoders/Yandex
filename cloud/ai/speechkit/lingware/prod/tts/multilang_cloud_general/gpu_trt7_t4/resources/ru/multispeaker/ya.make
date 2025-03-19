IF (NOT AUTOCHECK)

OWNER(g:cloud-asr)

UNION()

FILES(
    acoustic_resources_config.json
    encoder_config.json
    decoder_config.json
    vocoder_config.json
)

LARGE_FILES(
    acoustic_model/embeddings.npz
)

FROM_SANDBOX(FILE 3234731788 RENAME RESOURCE OUT vocoder/vocoder.trt)
FROM_SANDBOX(FILE 3234749251 RENAME RESOURCE OUT acoustic_model/encoder.trt)
FROM_SANDBOX(FILE 3234736673 RENAME RESOURCE OUT acoustic_model/decoder.trt)

END()

ENDIF()
