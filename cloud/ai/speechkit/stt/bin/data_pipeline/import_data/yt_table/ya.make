PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN yt_table.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/data_pipeline/import_data/records
    cloud/ai/speechkit/stt/lib/utils/s3
    cloud/ai/speechkit/stt/lib/text/re
    library/python/nirvana
)

END()
