OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/common/types/resources/query.sql ods/yt/startrek/common/types/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/common/types/resources/parameters.yaml ods/yt/startrek/common/types/resources/parameters.yaml
)

END()
