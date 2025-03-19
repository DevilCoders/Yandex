PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN revaluate_submission.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/tmp/ASREXP_778
    cloud/ai/speechkit/stt/lib/utils/s3
    cloud/ai/speechkit/stt/lib/data_pipeline/toloka
    contrib/python/requests
    library/python/nirvana
)

END()
