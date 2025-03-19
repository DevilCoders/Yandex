PY23_LIBRARY()

OWNER(g:cloud-dwh)


PEERDIR(
    contrib/python/pytz
    library/python/vault_client
)

END()

RECURSE(
    jobs
)
