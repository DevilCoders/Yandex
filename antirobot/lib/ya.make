OWNER(g:antirobot)

LIBRARY()

PEERDIR(
    antirobot/idl
    contrib/libs/openssl
    library/cpp/cgiparam
    library/cpp/charset
    library/cpp/containers/absl_flat_hash
    library/cpp/digest/md5
    library/cpp/digest/sfh
    library/cpp/fuid
    library/cpp/http/cookies
    library/cpp/http/io
    library/cpp/http/misc
    library/cpp/http/server
    library/cpp/ipv6_address
    library/cpp/iterator
    library/cpp/json
    library/cpp/lcookie
    library/cpp/logger
    library/cpp/neh
    library/cpp/regex/pire
    library/cpp/regex/regexp_classifier
    library/cpp/string_utils/base64
    library/cpp/string_utils/quote
    library/cpp/string_utils/scan
    library/cpp/threading/rcu
    yweb/webdaemons/icookiedaemon/icookie_lib/utils
    library/cpp/deprecated/atomic
)

SRCS(
    addr.cpp
    alarmer.cpp
    antirobot_response.cpp
    ar_utils.cpp
    bad_user_agents.cpp
    dynamic_thread_pool.cpp
    error.cpp
    evp.cpp
    fuid.cpp
    host_addr.cpp
    http_helpers.cpp
    http_request.cpp
    ip_hash.cpp
    ip_vec.cpp
    ip_interval.cpp
    ip_list.cpp
    ip_list_pid.cpp
    ip_map.cpp
    json.cpp
    keyring.cpp
    keyring_base.cpp
    kmp_skip_search.cpp
    log_utils.cpp
    mtp_queue_decorators.cpp
    neh_requester.cpp
    preemptive_mtp_queue.cpp
    preview_uid.cpp
    regex_detector.cpp
    regex_matcher.cpp
    reqtext_utils.cpp
    search_crawlers.cpp
    segv_handler.cpp
    spravka.cpp
    spravka_key.cpp
    stats_output.cpp
    stats_writer.cpp
    uri.cpp
    yandexuid.cpp
    yandex_trust_keyring.cpp
    yx_searchprefs.cpp
)

GENERATE_ENUM_SERIALIZATION(mini_geobase.h)

GENERATE_ENUM_SERIALIZATION(preview_uid.h)

GENERATE_ENUM_SERIALIZATION(search_crawlers.h)

GENERATE_ENUM_SERIALIZATION(stats_output.h)

GENERATE_ENUM_SERIALIZATION(stats_writer.h)

END()
