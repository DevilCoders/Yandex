OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/hosts/resources/query.sql ods/yt/mdb/hosts/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/hosts/resources/parameters.yaml ods/yt/mdb/hosts/resources/parameters.yaml
)

END()
