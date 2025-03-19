PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN run_compare_meaning.py
)

PEERDIR(
    cloud/ai/lib/python/datasource/yt/ops
    cloud/ai/speechkit/stt/lib/data_pipeline/toloka
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_quality
    contrib/python/ujson
    library/python/nirvana
)

END()
