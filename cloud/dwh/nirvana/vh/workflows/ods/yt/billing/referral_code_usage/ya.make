OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/referral_code_usage/resources/query.sql ods/yt/billing/referral_code_usage/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/referral_code_usage/resources/parameters.yaml ods/yt/billing/referral_code_usage/resources/parameters.yaml
)

END()
