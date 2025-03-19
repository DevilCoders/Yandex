OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/clusters/resources/query.sql ods/yt/mdb/clusters/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/clusters/resources/parameters.yaml ods/yt/mdb/clusters/resources/parameters.yaml
)

END()
