PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN produce_quality_evaluation_tasks.py
)

PEERDIR(
    contrib/python/ujson
    library/python/nirvana
    cloud/ai/speechkit/stt/lib/data/model
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_params
    cloud/ai/speechkit/stt/lib/data_pipeline/user_bits_urls
)

END()
