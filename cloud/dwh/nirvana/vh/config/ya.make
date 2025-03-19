OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    cloud/dwh/nirvana/config
)

PY_SRCS(
    __init__.py
    base.py
    preprod.py
    preprod_internal.py
    prod_internal.py
    prod.py
    uat.py
)

END()
