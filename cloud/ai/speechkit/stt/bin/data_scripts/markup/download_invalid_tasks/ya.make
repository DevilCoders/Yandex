PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN download_invalid_tasks.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/model
    cloud/ai/speechkit/stt/lib/data_pipeline/toloka
)

END()
