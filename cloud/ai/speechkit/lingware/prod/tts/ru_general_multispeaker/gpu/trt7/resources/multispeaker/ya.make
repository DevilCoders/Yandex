IF (NOT AUTOCHECK)

OWNER(g:cloud-asr)

UNION()

FILES(
    acoustic_resources_config.json
    encoder_config.json
    decoder_config.json
    vocoder_config.json
)

FROM_SANDBOX(
    2884298614 OUT
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
