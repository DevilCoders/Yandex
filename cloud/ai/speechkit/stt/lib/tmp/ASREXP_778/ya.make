PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PY_SRCS(
    __init__.py
    model.py
    tables.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
)

END()
