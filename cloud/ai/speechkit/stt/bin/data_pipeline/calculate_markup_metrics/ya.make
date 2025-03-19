PY3_PROGRAM()

OWNER(g:dataforge o-gulyaev)

PY_SRCS(
    MAIN calculate_markup_metrics.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_metrics
    contrib/python/ujson
    library/python/nirvana
)

END()
