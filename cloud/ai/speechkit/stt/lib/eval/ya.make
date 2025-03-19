PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PY_SRCS(
    metrics/calc/__init__.py
    metrics/calc/calc.py
    metrics/common/__init__.py
    metrics/common/metric.py
    metrics/levenshtein/__init__.py
    metrics/levenshtein/engine.py
    metrics/levenshtein/metric.py
    metrics/mer/__init__.py
    metrics/mer/engine.py
    metrics/mer/metric.py
    metrics/wer/__init__.py
    metrics/wer/metric.py
    metrics/wer/metric_ex.py
    metrics/__init__.py
    model/__init__.py
    model/model.py
    text/__init__.py
    text/common.py
    text/handler.py
    text/lemmatizer.py
    text/normalizer.py
    text/punctuation.py
    __init__.py
)

PEERDIR(
    contrib/python/ujson
    contrib/python/pymorphy2
    contrib/python/pylev

    alice/bitbucket/pynorm
    voicetech/asr/tools/asr_analyzer/lib

    cloud/ai/speechkit/stt/lib/text/slice/application
)

END()
