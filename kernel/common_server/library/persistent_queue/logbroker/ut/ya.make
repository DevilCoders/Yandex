UNITTEST_FOR(kernel/common_server/library/persistent_queue/logbroker)

OWNER(g:cs_dev)

SIZE(MEDIUM)

ENV(LOGBROKER_CREATE_TOPICS=StartStop,Read,Write,WriteRead,Ack,Multithread,Retry)
ENV(LOGBROKER_TOPICS_PARTITIONS=1)
ENV(PQ_OFFSET_RANGES_MODE="1")

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/lbk_recipe/recipe_stable.inc)

PEERDIR(
    library/cpp/testing/unittest
    library/cpp/yconf/patcher
)

SRCS(
    logbroker_ut.cpp
)

END()
