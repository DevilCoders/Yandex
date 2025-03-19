OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/organizations/resources/query.sql ods/yt/iam/organizations/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/organizations/resources/parameters.yaml ods/yt/iam/organizations/resources/parameters.yaml
)

END()
