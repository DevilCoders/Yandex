PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN old_pipeline_data.py
)

PEERDIR(
    contrib/python/boto3
    contrib/python/pydub
    contrib/python/pytz
    contrib/python/requests
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    cloud/ai/speechkit/stt/lib/data_pipeline/join
    cloud/ai/speechkit/stt/lib/data_pipeline/toloka
)

END()
