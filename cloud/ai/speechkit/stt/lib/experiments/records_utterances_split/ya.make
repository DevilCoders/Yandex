PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PY_SRCS(
    __init__.py
    split.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/eval
)

END()
