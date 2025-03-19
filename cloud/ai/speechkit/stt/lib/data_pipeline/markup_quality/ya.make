PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PY_SRCS(
    strategies/__init__.py
    strategies/default.py
    strategies/transcript.py
    __init__.py
    acceptance.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/model
    cloud/ai/speechkit/stt/lib/data_pipeline/toloka
    cloud/ai/speechkit/stt/lib/eval
)

END()
