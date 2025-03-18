G_BENCHMARK()

OWNER(g:antirobot)

SIZE(MEDIUM)

REQUIREMENTS(
    network:full
)

SRCS(
    main.cpp
    old_match_rules.cpp
)

PEERDIR(
    antirobot/daemon_lib
    library/cpp/iterator
    library/cpp/json
    library/cpp/string_utils/base64
)

DEPENDS(
    antirobot/scripts/gencfg
)

DATA(
    arcadia/antirobot/config/service_config.json
    arcadia/antirobot/config/service_identifier.json
    arcadia/antirobot/daemon_lib/benchmarks/rule_set/global_config.json
    arcadia/antirobot/scripts/gencfg/antirobot_gencfg
    sbr://3281797692 # antirobot data/formulas
    sbr://2479376555 # request data
    sbr://2793817192 # cbb.json dump
)

END()
