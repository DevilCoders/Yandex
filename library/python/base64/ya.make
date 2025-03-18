PY23_LIBRARY()

OWNER(pg)

PEERDIR(
    library/cpp/string_utils/base64
)

PY_SRCS(
    __init__.py
    __base64.pyx
)

END()
