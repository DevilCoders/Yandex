PY23_LIBRARY()

OWNER(
    g:passport_python
)

PEERDIR(
    contrib/python/paramiko
    contrib/python/requests
    contrib/python/six
)

SRCDIR(library/python/vault_client/vault_client)

PY_SRCS(
    __init__.py
    auth.py
    client.py
    errors.py
    instances.py
    utils.py
)

END()

RECURSE_FOR_TESTS(ut)
