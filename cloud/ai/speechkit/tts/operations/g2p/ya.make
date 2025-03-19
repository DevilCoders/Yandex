OWNER(g:cloud-asr)

PY3_PROGRAM(g2p)

PEERDIR(
    ml/nirvana/nope
)

PY_SRCS(
    operation.py
)

PY_MAIN(nope.entry_point)

END()
