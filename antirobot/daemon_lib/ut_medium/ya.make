UNITTEST()

OWNER(
    g:antirobot
)

PEERDIR(
    ADDINCL antirobot/daemon_lib
    antirobot/lib
)

SIZE(MEDIUM)
FORK_TESTS()

DATA(
    arcadia/antirobot/config/service_identifier.json
    arcadia/antirobot/config/service_config.json
    arcadia/antirobot/daemon_lib/ut/test_files
    sbr://758260112 # matrixnet.info
    sbr://2044897347 # catboost.info
    sbr://997562719 # geodata6-xurma.bin
)

SRCDIR(antirobot/daemon_lib)

SRCS(
    cacher_factors_ut.cpp
    captcha_check_ut.cpp
    captcha_page_ut.cpp
    catboost_ut.cpp
    environment_ut.cpp
    factors_ut.cpp
    fullreq_handler_ut.cpp
    instance_hashing_ut.cpp
    match_rules_ut.cpp
    request_classifier_ut.cpp
    request_features_ut.cpp
    request_params_ut.cpp
    robot_detector_ut.cpp
    rps_filter_ut.cpp
    user_reply_ut.cpp
)

REQUIREMENTS(ram:11)

END()
