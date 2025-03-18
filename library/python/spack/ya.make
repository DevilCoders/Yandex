OWNER(g:yatool g:logbroker)

RECURSE(
    tests
)

PY23_LIBRARY()

PEERDIR(
    library/cpp/monlib/encode/json
    library/cpp/monlib/encode/spack
    library/python/json
)

SRCS(
    wrapper.cpp
)

PY_SRCS(
    __init__.py
    convert.pyx
)

END()
