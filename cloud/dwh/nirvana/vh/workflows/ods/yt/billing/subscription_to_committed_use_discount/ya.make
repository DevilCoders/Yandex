OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/subscription_to_committed_use_discount/resources/query.sql ods/yt/billing/subscription_to_committed_use_discount/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/subscription_to_committed_use_discount/resources/parameters.yaml ods/yt/billing/subscription_to_committed_use_discount/resources/parameters.yaml
)

END()
