OWNER(g:cloud-asr)

UNION()

FILES(
    asr_system_config.json
    features_config.json
    resources_spec.json
    he/am_config.json
    he/letters.lst
)

FROM_SANDBOX(FILE 3290895224 RENAME RESOURCE OUT he/jasper.trt)

END()
