PY3_PROGRAM(jwt-generator)

OWNER(g:cloud-iam)

PEERDIR(
    contrib/python/PyJWT
)

PY_MAIN(jwt-generator)

PY_SRCS(
    TOP_LEVEL
    jwt-generator.py
)

END()
