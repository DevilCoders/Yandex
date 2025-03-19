PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN select_unmarked_records.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_params
    cloud/ai/speechkit/stt/lib/utils/s3
    contrib/python/ujson
    library/python/nirvana
)

END()
