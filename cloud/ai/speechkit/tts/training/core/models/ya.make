OWNER(g:cloud-asr)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    embedding.py
    positional_encoding.py
    transformer.py
)

NO_CHECK_IMPORTS()

END()
