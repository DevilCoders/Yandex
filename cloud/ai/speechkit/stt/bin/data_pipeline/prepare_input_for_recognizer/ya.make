PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN prepare_input_for_recognizer.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/model
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    contrib/python/ujson
    library/python/nirvana
)

END()
