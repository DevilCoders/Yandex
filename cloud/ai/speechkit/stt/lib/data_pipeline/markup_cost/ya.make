PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/transcription_tasks
    cloud/ai/speechkit/stt/lib/data_pipeline/records_splitting
)

PY_SRCS(
    __init__.py
    markup_cost.py
)

END()
