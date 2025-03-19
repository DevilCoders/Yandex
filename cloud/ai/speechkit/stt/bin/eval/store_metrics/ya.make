PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN store_metrics.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/text/slice/generation
    contrib/python/ujson
    library/python/nirvana
)

END()
