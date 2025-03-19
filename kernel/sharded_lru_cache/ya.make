LIBRARY()

OWNER(ayles)

SRC(
    sharded_lru_cache.cpp
)

PEERDIR(
    library/cpp/cache
    kernel/sharded_lru_cache/proto
)

END()
