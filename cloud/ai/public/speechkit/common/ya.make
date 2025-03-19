PY3_LIBRARY()

OWNER(
    g:cloud-asr
)

PY_SRCS(
    NAMESPACE speechkit.common
    __init__.py
)

END()

RECURSE(utils)
