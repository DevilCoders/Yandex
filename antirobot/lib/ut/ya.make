OWNER(g:antirobot)

UNITTEST()

REQUIREMENTS(network:full)

PEERDIR(
    ADDINCL antirobot/lib
    ADDINCL antirobot/lib/p0f_parser
    ADDINCL antirobot/lib/ja3_parser
    library/cpp/charset
    library/cpp/digest/sfh
    library/cpp/fuid
    library/cpp/http/cookies
    library/cpp/http/io
    library/cpp/http/misc
    library/cpp/http/server
    library/cpp/json
    library/cpp/lcookie
    library/cpp/regex/pire
    library/cpp/string_utils/base64
    library/cpp/threading/rcu
    yweb/webdaemons/icookiedaemon/icookie_lib/utils
)

SRCDIR(antirobot/lib)

SRCS(
    addr_ut.cpp
    alarmer_ut.cpp
    ar_utils_ut.cpp
    bad_user_agents_ut.cpp
    data_process_queue_ut.cpp
    evp_ut.cpp
    fuid_ut.cpp
    host_addr_ut.cpp
    http_helpers_ut.cpp
    http_request_ut.cpp
    ip_hash_ut.cpp
    ip_interval_ut.cpp
    ip_list_pid_ut.cpp
    ip_list_ut.cpp
    ip_map_ut.cpp
    keyring_ut.cpp
    kmp_skip_search_ut.cpp
    mtp_queue_decorators_ut.cpp
    neh_requester_ut.cpp
    preview_uid_ut.cpp
    regex_detector_ut.cpp
    search_crawlers_ut.cpp
    spravka_ut.cpp
    stats_output_ut.cpp
    stats_writer_ut.cpp
    yandexuid_ut.cpp
    yx_searchprefs_ut.cpp
    p0f_parser/parser_ut.cpp
    ja3_parser/parser_ut.cpp
)

END()
