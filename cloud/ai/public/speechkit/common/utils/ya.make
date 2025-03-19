OWNER(g:cloud-asr)

PY3_LIBRARY()

PEERDIR(
    contrib/python/boto3
)

PY_SRCS(
    NAMESPACE speechkit.common.utils
    __init__.py
    utils.py
)

END()
