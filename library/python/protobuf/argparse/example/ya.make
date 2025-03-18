PY3_PROGRAM()

OWNER(
    mstebelev
    g:contrib
)

PY_SRCS(
    script.py
    conf.proto
)

PY_MAIN(library.python.protobuf.argparse.example.script)

PEERDIR(
    library/python/protobuf/argparse
)

END()
