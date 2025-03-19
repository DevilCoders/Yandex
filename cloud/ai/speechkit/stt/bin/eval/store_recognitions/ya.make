PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN store_recognitions.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    contrib/python/ujson
    library/python/nirvana
)

END()
