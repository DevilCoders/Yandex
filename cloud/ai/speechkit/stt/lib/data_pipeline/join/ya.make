PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/model
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_params
    cloud/ai/speechkit/stt/lib/data_pipeline/transcript_feedback_loop
)

PY_SRCS(
    __init__.py
    common.py
    io.py
    join.py
    majority.py
)

END()
