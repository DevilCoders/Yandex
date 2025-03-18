PY23_LIBRARY()

OWNER(
    g:passport_python
)

PEERDIR(
    contrib/python/mock
    contrib/python/paramiko
    contrib/python/requests-mock
    library/python/vault_client
)

TEST_SRCS(
    __init__.py
    data.py
    test.py
)



END()
