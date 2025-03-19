PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN produce_transcription_tasks.py
)

PEERDIR(
    contrib/python/ujson
    cloud/ai/speechkit/stt/lib/data/model
    cloud/ai/speechkit/stt/lib/data_pipeline/transcription_tasks
    library/python/nirvana
)

END()
