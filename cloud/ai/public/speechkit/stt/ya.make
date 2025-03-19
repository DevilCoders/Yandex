OWNER(noxoomo)

PY3_LIBRARY()

PEERDIR(
    cloud/bitbucket/public-api/yandex/cloud/ai/stt/v2
    cloud/bitbucket/public-api/yandex/cloud/ai/stt/v3
    cloud/ai/public/speechkit/common
    cloud/ai/public/speechkit/common/utils
    contrib/python/pydub
    contrib/python/boto3
    contrib/python/requests
)

PY_SRCS(
    NAMESPACE speechkit.stt
    __init__.py
    transcription.py
    recognizer.py
    azure/__init__.py
    azure/model.py
    yandex/__init__.py
    yandex/model.py
)

END()
