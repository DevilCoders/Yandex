IF (NOT AUTOCHECK)

OWNER(g:cloud-asr)

UNION()

FILES(
    text_preprocessor_config.json
    acoustic_resources_config.json
    encoder_config.json
    decoder_config.json
)

FROM_SANDBOX(
    2861853305 OUT
    acoustic_model/embeddings.npz
    acoustic_model/encoder.trt
    acoustic_model/decoder.trt
)

END()

ENDIF()
