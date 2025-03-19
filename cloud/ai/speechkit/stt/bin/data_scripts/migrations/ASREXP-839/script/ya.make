PY3_PROGRAM()

OWNER(g:dataforge o-gulyaev)

PY_SRCS(
    MAIN script.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_metrics
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_quality
    cloud/ai/speechkit/stt/lib/data_pipeline/files
)

END()
