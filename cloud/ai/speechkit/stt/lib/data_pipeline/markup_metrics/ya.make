PY3_LIBRARY()

OWNER(g:dataforge o-gulyaev)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_quality
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_params
    cloud/ai/speechkit/stt/lib/data_pipeline/transcript_feedback_loop
    contrib/python/numpy
)

PY_SRCS(
    __init__.py
    markup_metrics.py
)

END()
