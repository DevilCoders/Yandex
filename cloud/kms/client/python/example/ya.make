PY3_PROGRAM(kms_example_python)

OWNER(g:cloud-kms)

PEERDIR(
    cloud/kms/client/python
)

PY_SRCS(
    MAIN main.py
)

END()
