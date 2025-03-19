OWNER(yurakura)

UNITTEST_FOR(kernel/geodb/translocal)

FORK_SUBTESTS()

DATA(sbr://103186702)

SRCS(
    translocal_ut.cpp
)

PEERDIR(
    kernel/geodb
    kernel/relev_locale/protos
)

END()
