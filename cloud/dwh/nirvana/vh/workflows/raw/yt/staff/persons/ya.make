OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    cloud/dwh/nirvana/vh/common
    contrib/python/requests
    nirvana/valhalla/src
    yt/python/client
)

PY_SRCS(
    __init__.py
)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/raw/yt/staff/persons/resources/script.py raw/yt/staff/persons/resources/script.py
    cloud/dwh/nirvana/vh/workflows/raw/yt/staff/persons/resources/parameters.yaml raw/yt/staff/persons/resources/parameters.yaml
)

END()