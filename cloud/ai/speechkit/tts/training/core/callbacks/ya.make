OWNER(g:cloud-asr)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    base.py
    model_checkpoint.py
    nirvana_checkpoint.py
)

NO_CHECK_IMPORTS()

END()
