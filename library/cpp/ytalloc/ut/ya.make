UNITTEST()

SIZE(MEDIUM)

OWNER(g:yt)

ALLOCATOR(YT)

SRCS(
    ut.cpp
)

PYTHON(
    make_blob.py
    STDOUT_NOAUTO data.blob
)

# Artificially increase binary size
RESOURCE(data.blob ytalloc/debug)

PEERDIR(
    library/cpp/testing/benchmark
)

REQUIREMENTS(ram:13)

END()
