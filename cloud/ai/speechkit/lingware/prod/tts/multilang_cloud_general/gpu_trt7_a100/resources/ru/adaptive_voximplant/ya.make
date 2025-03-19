IF (NOT AUTOCHECK)

OWNER(g:cloud-asr)

UNION()

FILES(
    acoustic_resources_config.json
    template_helper_config.json
    encoder_config.json
    decoder_config.json
    vocoder_config.json
)

FROM_SANDBOX(
    3359018961 OUT
    acoustic_model/embeddings.npz
    acoustic_model/gap_embedding.npz
    acoustic_model/encoder.trt
    acoustic_model/decoder.trt
)

FROM_SANDBOX(
    3359039741 OUT
    vocoder/vocoder.trt
)

END()

ENDIF()
