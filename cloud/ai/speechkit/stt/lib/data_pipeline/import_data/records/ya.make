PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    contrib/python/pydub
    contrib/python/scipy
)

PY_SRCS(
    __init__.py
    records.py
)

END()
