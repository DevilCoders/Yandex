PY2_PROGRAM(decrypt_url_configs)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/pyre2
    contrib/python/tenacity
    contrib/python/requests
    library/python/tvmauth
    library/python/vault_client
    antiadblock/tasks/tools
)

END()
