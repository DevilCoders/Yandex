UNITTEST()

OWNER(
    g:antirobot
)

PEERDIR(
    ADDINCL antirobot/daemon_lib
    antirobot/captcha
    antirobot/idl
    antirobot/lib
    metrika/uatraits/library
    metrika/uatraits/data
    infra/yp_service_discovery/libs/sdlib/server_mock
    kernel/geo
    kernel/xmlreq
    library/cpp/archive
    library/cpp/charset
    library/cpp/containers/sorted_vector
    library/cpp/eventlog
    library/cpp/http/cookies
    library/cpp/http/io
    library/cpp/http/misc
    library/cpp/http/server
    library/cpp/iterator
    library/cpp/langs
    library/cpp/regex/regexp_classifier
    library/cpp/threading/rcu
    library/cpp/yconf
    search/wizard/config
    yweb/webdaemons/icookiedaemon/icookie_lib/utils
)


   YQL_LAST_ABI_VERSION()


REQUIREMENTS(network:full)

DATA(
    arcadia/antirobot/config/service_identifier.json
    arcadia/antirobot/config/service_config.json
    arcadia/antirobot/daemon_lib/factors_versions/
    sbr://758260112 # matrixnet.info
    sbr://2033622829 # catboost.info
)

SRCDIR(antirobot/daemon_lib)

SRCS(
    addr_list_ut.cpp
    antirobot_cookie_ut.cpp
    autoru_offer_ut.cpp
    autoru_tamper_ut.cpp
    backend_sender_ut.cpp
    block_set_ut.cpp
    captcha_fury_check_ut.cpp
    captcha_key_ut.cpp
    captcha_parse_ut.cpp
    cbb_banned_ip_holder_ut.cpp
    config_ut.cpp
    convert_factors_ut.cpp
    cyclic_queue_ut.cpp
    disabling_flags_ut.cpp
    factor_names_ut.cpp
    fullreq_info_ut.cpp
    host_ops_ut.cpp
    jsonp_ut.cpp
    match_rule_lexer_ut.cpp
    neh_requesters_ut.cpp
    night_check_ut.cpp
    page_template_ut.cpp
    panic_flags_ut.cpp
    parse_cbb_response_ut.cpp
    req_types_ut.cpp
    request_time_stats_ut.cpp
    return_path_ut.cpp
    robot_set_ut.cpp
    rps_calc_ut.cpp
    search_engine_recognizer_ut.cpp
    uid_ut.cpp
    xml_reqs_helpers_ut.cpp
    yql_rule_set_ut.cpp
)

REQUIREMENTS(ram:32)

END()
