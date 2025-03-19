OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/grant_policies/resources/query.sql ods/yt/billing/grant_policies/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/grant_policies/resources/parameters.yaml ods/yt/billing/grant_policies/resources/parameters.yaml
)

END()
