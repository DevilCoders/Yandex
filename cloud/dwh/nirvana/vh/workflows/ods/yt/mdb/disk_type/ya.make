OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/disk_type/resources/query.sql ods/yt/mdb/disk_type/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/disk_type/resources/parameters.yaml ods/yt/mdb/disk_type/resources/parameters.yaml
)

END()
