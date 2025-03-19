IF (NOT AUTOCHECK)

OWNER(g:cloud-asr)

UNION()

FILES(
    acoustic_resources_config.json
    encoder_config.json
    decoder_config.json
)

LARGE_FILES(
    acoustic_model/encoder.trt
    acoustic_model/decoder.trt
    acoustic_model/embeddings.npz
)

END()

ENDIF()
