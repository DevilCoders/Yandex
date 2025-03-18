LIBRARY()

OWNER(agri)

SRCS(
    lightrwlock.cpp
    lightrwlock.h
)

END()

RECURSE(
    bench
    ut
)
