LIBRARY()

OWNER(pg)

# TODO
NO_COMPILER_WARNINGS()

PEERDIR(
    contrib/libs/c-ares
    library/cpp/coroutine/engine
    library/cpp/cache
)

SRCS(
    async.cpp
    cache.cpp
    coro.cpp
    helpers.cpp
)

END()
