LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/regex/pcre
    library/cpp/cgiparam
    library/cpp/xml/document
    library/cpp/object_factory
    kernel/common_server/library/geometry
    kernel/common_server/abstract
    kernel/common_server/library/logging
    kernel/common_server/report
    kernel/common_server/library/network/data
    kernel/common_server/library/searchserver/simple/context
    kernel/common_server/library/scheme
    library/cpp/deprecated/atomic
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(pagination.h)

SRCS(
    abstract.cpp
    manager_config.cpp
    attributed.cpp
    host_filter.cpp
    params_processor.cpp
    processor.cpp
    url_matcher.cpp
    GLOBAL scheme.cpp
)

GENERATE_ENUM_SERIALIZATION(scheme.h)

GENERATE_ENUM_SERIALIZATION(processor.h)

END()

RECURSE(
    ut
)
