PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN receive_submission.py
)

PEERDIR(
    cloud/ai/lib/python/datasource/yt/ops
    cloud/ai/speechkit/stt/lib/tmp/ASREXP_778
    cloud/ai/speechkit/stt/lib/utils/s3
    library/python/nirvana
)

END()
