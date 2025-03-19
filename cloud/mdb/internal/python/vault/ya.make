PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/retrying
    library/python/vault_client
)

ALL_PY_SRCS()

END()
