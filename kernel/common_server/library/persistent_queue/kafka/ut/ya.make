UNITTEST_FOR(kernel/common_server/library/persistent_queue/kafka)

OWNER(g:cs_dev)

SIZE(MEDIUM)

ENV(KAFKA_CREATE_TOPICS=StartStop,Read,Write,WriteRead,Ack)
ENV(KAFKA_TOPICS_PARTITIONS=1)

INCLUDE(${ARCADIA_ROOT}/library/recipes/kafka/recipe.inc)

PEERDIR(
    library/cpp/testing/unittest
    library/cpp/yconf/patcher
)

SRCS(
    kafka_ut.cpp
)

REQUIREMENTS(ram:10)

END()
