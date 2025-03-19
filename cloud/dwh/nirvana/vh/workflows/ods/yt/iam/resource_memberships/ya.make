OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/resource_memberships/resources/query.sql ods/yt/iam/resource_memberships/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/resource_memberships/resources/parameters.yaml ods/yt/iam/resource_memberships/resources/parameters.yaml
)

END()
