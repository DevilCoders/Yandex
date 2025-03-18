OWNER(
    knoxja
    g:antiinfra
)

PY2_LIBRARY()

PY_SRCS(
    NAMESPACE etcd
    __init__.py
    auth.py
    client.py
    lock.py
)

PEERDIR(
   contrib/python/dnspython
)

END()
