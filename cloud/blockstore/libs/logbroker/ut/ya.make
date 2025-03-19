UNITTEST_FOR(cloud/blockstore/libs/logbroker)

OWNER(g:cloud-nbs)

SRCS(
    logbroker_ut.cpp
)

PEERDIR(
    library/cpp/testing/unittest
)

ENV(YDB_USE_IN_MEMORY_PDISKS=true)
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/lbk_recipe/recipe_stable.inc)

END()
