LIBRARY()

OWNER(iddqd)

SRCS(
    data_set.cpp
    processor_config.cpp
    processor.cpp
    source.cpp
    converter.cpp
    sender.cpp
    link.cpp
    link_config.cpp
    counter.cpp
    replier.cpp
    trace_replier.cpp
    replier_decorator.cpp
)

GENERATE_ENUM_SERIALIZATION(data_set.h)

PEERDIR(
    kernel/common_proxy/unistat_signals
    kernel/daemon
    kernel/daemon/module
    library/cpp/bucket_quoter
    library/cpp/histogram/rt
    library/cpp/logger/global
    library/cpp/object_factory
    library/cpp/yconf
)

END()
