UNITTEST_FOR(kernel/shard_conf)

OWNER(
    velavokr
    kostik
    g:base
)

PEERDIR(
    kernel/shard_conf
    library/cpp/resource
)

SRCS(
    shard_conf_ut.cpp
)

RESOURCE(
    kernel/shard_conf/ut/basesearch.shard.conf /basesearch.shard.conf
    kernel/shard_conf/ut/querysearch.shard.conf /querysearch.shard.conf
)

END()
