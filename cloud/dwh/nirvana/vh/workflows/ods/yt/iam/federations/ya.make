OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/federations/resources/query.sql ods/yt/iam/federations/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/federations/resources/parameters.yaml ods/yt/iam/federations/resources/parameters.yaml
)

END()
