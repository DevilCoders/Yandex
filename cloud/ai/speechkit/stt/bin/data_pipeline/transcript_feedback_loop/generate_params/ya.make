PY3_PROGRAM()

PY_SRCS(
    MAIN generate_params.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_params
    library/python/nirvana
)

END()
