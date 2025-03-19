PY3_PROGRAM()
OWNER(
    jellysnack
)
PY_SRCS(
    MAIN download_records.py
)
PEERDIR(
    contrib/python/boto3
    contrib/python/pydub
    library/python/nirvana
    cloud/ai/speechkit/stt/lib/experiments/audio
)
END()
