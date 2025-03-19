LIBRARY()

OWNER(g:cs_dev)

SRCS(
    GLOBAL docinfo.cpp
    GLOBAL factornames.cpp
    GLOBAL histogram.cpp
    GLOBAL indexstat_calc.cpp
)

PEERDIR(
    library/cpp/json
    library/cpp/histogram/rt
    search/meta
    search/meta/scatter
)

END()
