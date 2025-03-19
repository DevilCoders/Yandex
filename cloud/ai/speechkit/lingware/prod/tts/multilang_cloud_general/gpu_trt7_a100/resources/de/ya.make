IF (NOT AUTOCHECK)

OWNER(g:cloud-asr)

UNION()

FILES(
    chunker_config.json
    normalizer_config.json
    text_preprocessor_config.json
)

END()

ENDIF()

RECURSE(
    acoustic_model
)
