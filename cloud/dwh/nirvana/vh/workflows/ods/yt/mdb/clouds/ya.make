OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/clouds/resources/query.sql ods/yt/mdb/clouds/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/clouds/resources/parameters.yaml ods/yt/mdb/clouds/resources/parameters.yaml
)

END()
