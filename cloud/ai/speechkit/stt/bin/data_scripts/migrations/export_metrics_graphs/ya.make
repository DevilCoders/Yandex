PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN export_metrics_graphs.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    contrib/python/requests
)

END()
