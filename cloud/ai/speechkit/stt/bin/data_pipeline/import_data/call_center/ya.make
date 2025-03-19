PY3_PROGRAM()

OWNER(
    hotckiss
)

PY_SRCS(
    MAIN import_call_center.py
)

PEERDIR(
    contrib/python/pydub
    contrib/python/pytz
    contrib/python/requests
    contrib/python/ffmpeg-python
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/data_pipeline/import_data/records
    cloud/ai/speechkit/stt/lib/utils/s3
)

END()
