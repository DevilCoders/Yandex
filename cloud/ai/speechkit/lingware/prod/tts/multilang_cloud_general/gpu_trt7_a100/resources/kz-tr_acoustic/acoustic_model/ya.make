IF (NOT AUTOCHECK)

OWNER(g:cloud-asr)

UNION()

FILES(
    acoustic_resources_config.json
    encoder_config.json
    decoder_config.json
)

FROM_SANDBOX(
    3162403567
    RENAME acoustic_model/embeddings.npz OUT_NOAUTO resources/embeddings.npz
    RENAME acoustic_model/encoder.trt OUT_NOAUTO resources/encoder.trt
    RENAME acoustic_model/decoder.trt OUT_NOAUTO resources/decoder.trt
)

END()

ENDIF()
