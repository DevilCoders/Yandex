UNITTEST_FOR(library/cpp/threading/light_rw_lock)

OWNER(agri)

SRCS(
    rwlock_ut.cpp
)

PEERDIR(
    library/cpp/deprecated/atomic
)

END()
