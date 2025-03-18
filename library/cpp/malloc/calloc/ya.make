LIBRARY()

OWNER(iddqd)

NO_COMPILER_WARNINGS()

PEERDIR(
    library/cpp/balloc_market/lib # TODO(danlark@) EXTREMELY UGLY
    library/cpp/malloc/api
    library/cpp/malloc/calloc/options
)

CXXFLAGS(-DLFALLOC_YT)

SRCS(
    calloc.cpp
)

END()
