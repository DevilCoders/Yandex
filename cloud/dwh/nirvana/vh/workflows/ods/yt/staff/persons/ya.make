OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/staff/persons/resources/query.sql ods/yt/staff/persons/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/staff/persons/resources/parameters.yaml ods/yt/staff/persons/resources/parameters.yaml
)

END()
