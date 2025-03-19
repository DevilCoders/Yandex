PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN prepare_aligner_input.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/model
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    contrib/python/pydub
    library/python/nirvana
)

END()
