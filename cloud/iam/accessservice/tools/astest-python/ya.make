PY2_PROGRAM(astest)

OWNER(g:cloud-iam)

PEERDIR(
    cloud/iam/accessservice/client/python
)

PY_MAIN(astest)

PY_SRCS(
    TOP_LEVEL
    astest.py
)

END()
