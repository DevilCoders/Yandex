PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN calculate_metrics.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/model
    cloud/ai/speechkit/stt/lib/eval
    contrib/python/ujson
    library/python/nirvana
)

END()
