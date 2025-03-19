PY3_PROGRAM(configs-deployer)
OWNER(g:music-sre)

PY_SRCS(MAIN deployer.py)

PEERDIR(
    contrib/python/requests
)

END()
