PY3_LIBRARY()

OWNER(eranik)

SRCDIR(cloud/ai/datasphere/lib/stt/metrics/python_package/stt_metrics)

PEERDIR(
    contrib/python/numpy
    contrib/python/pymorphy2
    cloud/ai/datasphere/lib/stt/metrics/python_package/stt_metrics/normalizer_service
)

NO_COMPILER_WARNINGS()

PY_SRCS(
    NAMESPACE stt_metrics
    __init__.py
    alignment_utils.pyx
    common.py
    resources.py
    text_transform.py
    version.py
    wer.py
)

END()
