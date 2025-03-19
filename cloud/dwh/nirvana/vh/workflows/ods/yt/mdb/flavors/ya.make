OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/flavors/resources/query.sql ods/yt/mdb/flavors/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/flavors/resources/parameters.yaml ods/yt/mdb/flavors/resources/parameters.yaml
)

END()
