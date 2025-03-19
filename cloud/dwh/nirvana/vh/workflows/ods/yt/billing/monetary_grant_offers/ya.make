OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/monetary_grant_offers/resources/query.sql ods/yt/billing/monetary_grant_offers/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/monetary_grant_offers/resources/parameters.yaml ods/yt/billing/monetary_grant_offers/resources/parameters.yaml
)

END()
