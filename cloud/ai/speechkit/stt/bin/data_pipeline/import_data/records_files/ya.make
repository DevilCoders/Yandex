PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN records_files.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/data_pipeline/import_data/records
    cloud/ai/speechkit/stt/lib/utils/s3
)

END()
