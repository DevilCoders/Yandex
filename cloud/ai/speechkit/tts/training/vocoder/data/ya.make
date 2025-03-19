OWNER(g:cloud-asr)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    batch.py
    features_extractor.py
    sample_builder.py
)

NO_CHECK_IMPORTS()

END()
