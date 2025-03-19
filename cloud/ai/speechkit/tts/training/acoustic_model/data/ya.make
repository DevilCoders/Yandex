OWNER(g:cloud-asr)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    batch.py
    masking.py
    sample_builder.py
)

NO_CHECK_IMPORTS()

END()

RECURSE(
    text_processor
)
