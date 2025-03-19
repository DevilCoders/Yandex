OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts/resources/query.sql ods/yt/billing/billing_accounts/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts/resources/parameters.yaml ods/yt/billing/billing_accounts/resources/parameters.yaml
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts/resources/utils/billing_accounts.sql ods/yt/billing/billing_accounts/resources/utils/billing_accounts.sql
)

END()
