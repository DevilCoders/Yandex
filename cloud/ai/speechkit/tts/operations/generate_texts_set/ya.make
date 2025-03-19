OWNER(g:cloud-asr)

PY3_PROGRAM(generate_texts_set)

PEERDIR(
    ml/nirvana/nope
    yt/python/client
)

PY_SRCS(
    operation.py
)

PY_MAIN(nope.entry_point)

END()
