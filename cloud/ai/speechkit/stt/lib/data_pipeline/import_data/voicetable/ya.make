PY3_LIBRARY()

OWNER(
    hotckiss
)

PEERDIR(
    contrib/python/boto3
    contrib/python/pydub
    contrib/python/pytz
    contrib/python/requests
    contrib/python/ffmpeg-python
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/data_pipeline/import_data/records
)

PY_SRCS(
    __init__.py
    voicetable.py
)

END()
