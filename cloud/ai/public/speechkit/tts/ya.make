OWNER(g:cloud-asr)

PY3_LIBRARY()

PEERDIR(
    cloud/bitbucket/public-api/yandex/cloud/ai/tts/v3
    cloud/ai/public/speechkit/common/utils
    contrib/python/pydub
)

PY_SRCS(
    NAMESPACE speechkit.tts
    __init__.py
    model.py
)

END()
