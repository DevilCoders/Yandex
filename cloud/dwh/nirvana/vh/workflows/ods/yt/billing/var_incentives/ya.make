OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/var_incentives/resources/query.sql ods/yt/billing/var_incentives/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/var_incentives/resources/parameters.yaml ods/yt/billing/var_incentives/resources/parameters.yaml
)

END()
