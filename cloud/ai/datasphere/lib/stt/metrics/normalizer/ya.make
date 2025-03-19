OWNER(eranik)

PY3_PROGRAM(run_service)

PEERDIR(
    cloud/ai/datasphere/lib/stt/metrics/normalizer/proto
    cloud/ai/speechkit/stt/lib/eval
)

PY_SRCS(
    MAIN service.py
)

END()

RECURSE(
    resources
)
