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
    acoustic_model/encoder.trt
    acoustic_model/decoder.trt
)

FROM_SANDBOX(
    2861880947 OUT
    vocoder/vocoder.trt
)

END()

ENDIF()
