PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN produce_check_transcription_tasks.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/model
    contrib/python/ujson
    library/python/nirvana
)

END()
