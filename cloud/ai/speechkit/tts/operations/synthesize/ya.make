OWNER(g:cloud-asr)

PY3_PROGRAM(synthesize)

PEERDIR(
    ml/nirvana/nope
)

PY_SRCS(
    operation.py
)

PY_MAIN(nope.entry_point)

END()
