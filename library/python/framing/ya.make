PY23_LIBRARY()

OWNER(
    rmcf
    g:yabs-rt
)

PEERDIR(
    contrib/python/protobuf
    library/cpp/framing
)

PY_SRCS(
    _format.pyx
    _packer.pyx
    _unpacker.pyx
    format.py
    packer.py
    unpacker.py
)

END()

RECURSE_FOR_TESTS(ut)
