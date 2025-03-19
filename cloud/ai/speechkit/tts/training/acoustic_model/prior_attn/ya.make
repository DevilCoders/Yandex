OWNER(g:cloud-asr)

PY3_LIBRARY()

FILES(
    prior_attn.cu
)

PY_SRCS(
    __init__.py
    setup.py
)

NO_CHECK_IMPORTS()

END()
