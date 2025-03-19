PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN upload_audio_for_toloka.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/utils/s3
    library/python/nirvana
)

END()
