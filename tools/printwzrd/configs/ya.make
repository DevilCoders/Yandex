UNION()

OWNER(gluk47)

RUN_PROGRAM(
    search/wizard/data/wizard/conf/executable --quiet --shard-prefix wizard --printwizard ${ARCADIA_BUILD_ROOT}/tools/printwzrd/configs
    OUT_NOAUTO
        antirobot-yaml.cfg
        geo-printwizard-yaml.cfg
        geo-yaml.cfg
        rsya-yaml.cfg
        wizard-yaml.cfg
    STDOUT printwizard.log
)

END()
