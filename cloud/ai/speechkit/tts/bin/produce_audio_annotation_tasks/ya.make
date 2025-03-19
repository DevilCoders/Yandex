PY3_PROGRAM()

OWNER(
    luda-gordeeva
)

PY_SRCS(
    MAIN produce_audio_annotation_tasks.py
)

PEERDIR(
    library/python/nirvana
    yt/python/client
)

END()
