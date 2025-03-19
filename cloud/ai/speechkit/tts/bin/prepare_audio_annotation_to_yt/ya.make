PY3_PROGRAM()

OWNER(
    luda-gordeeva
)

PY_SRCS(
    MAIN prepare_audio_annotation_to_yt.py
)

PEERDIR(
    library/python/nirvana
    yt/python/client
    cloud/ai/lib/python/datasource/yt/model
    cloud/ai/lib/python/datetime
    cloud/ai/lib/python/serialization
)

END()
