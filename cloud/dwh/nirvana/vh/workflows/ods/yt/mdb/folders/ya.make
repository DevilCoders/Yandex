OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/folders/resources/query.sql ods/yt/mdb/folders/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/folders/resources/parameters.yaml ods/yt/mdb/folders/resources/parameters.yaml
)

END()
