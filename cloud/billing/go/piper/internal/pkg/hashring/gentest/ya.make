PY3_PROGRAM()
OWNER(g:cloud-billing)

PY_MAIN(hashring:main)

PEERDIR(
    contrib/python/uhashring
)

PY_SRCS(
    TOP_LEVEL
	hashring.py
)

END()
