IF (NOT AUTOCHECK)

OWNER(g:cloud-asr)

UNION()

FILES(
    acoustic_resources_config.json
    encoder_config.json
    decoder_config.json
)

FROM_SANDBOX(
    3238818681
    RENAME acoustic_model/embeddings/embeddings.npz OUT_NOAUTO resources/embeddings.npz
    RENAME acoustic_model/trt/encoder.trt OUT_NOAUTO resources/encoder.trt
    RENAME acoustic_model/trt/decoder.trt OUT_NOAUTO resources/decoder.trt
)

END()

ENDIF()
