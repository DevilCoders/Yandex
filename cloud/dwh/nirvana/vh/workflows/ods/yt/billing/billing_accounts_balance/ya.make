OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts_balance/resources/query.sql ods/yt/billing/billing_accounts_balance/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts_balance/resources/parameters.yaml ods/yt/billing/billing_accounts_balance/resources/parameters.yaml
)

END()
