PY3_LIBRARY()

OWNER(
    g:cloud-asr
)


PEERDIR(
    cloud/ai/public/speechkit/common
    cloud/ai/public/speechkit/stt
    cloud/ai/public/speechkit/tts
)

# have to disable them because cython's numpy integration uses deprecated numpy API
NO_COMPILER_WARNINGS()

PY_SRCS(
    NAMESPACE speechkit
    __init__.py
    model_repository.py
    version.py
)

END()

RECURSE(
    common
    stt
    tts
)
