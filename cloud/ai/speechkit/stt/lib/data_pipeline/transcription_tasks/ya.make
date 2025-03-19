PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PY_SRCS(
    __init__.py
    transcription_tasks.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/files
)

END()
