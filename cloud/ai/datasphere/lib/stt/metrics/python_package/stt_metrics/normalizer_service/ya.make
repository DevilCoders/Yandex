OWNER(eranik)

PY3_LIBRARY()

PEERDIR(
    cloud/ai/datasphere/lib/stt/metrics/normalizer/proto
)

PY_SRCS(
    NAMESPACE stt_metrics.normalizer_service
    __init__.py
)

END()
