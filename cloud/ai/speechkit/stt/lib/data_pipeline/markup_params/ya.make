PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PY_SRCS(
    __init__.py
    markup_params.py
)

PEERDIR(
    dataforge
    cloud/ai/speechkit/stt/lib/data/model
    library/python/toloka-kit
)

END()
