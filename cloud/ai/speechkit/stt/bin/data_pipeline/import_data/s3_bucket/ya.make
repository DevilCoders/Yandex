PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN s3_bucket.py
)

PEERDIR(
    cloud/ai/lib/python/datasource/yql
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/data_pipeline/import_data/records
    cloud/ai/speechkit/stt/lib/utils/s3
    library/python/nirvana
)

END()
