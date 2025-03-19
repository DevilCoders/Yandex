OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/billing_accounts/resources/query.sql ods/yt/crm/billing_accounts/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/billing_accounts/resources/parameters.yaml ods/yt/crm/billing_accounts/resources/parameters.yaml
)

END()
