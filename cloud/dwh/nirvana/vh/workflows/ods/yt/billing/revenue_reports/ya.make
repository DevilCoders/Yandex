OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/revenue_reports/resources/query.sql ods/yt/billing/revenue_reports/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/revenue_reports/resources/parameters.yaml ods/yt/billing/revenue_reports/resources/parameters.yaml
)

END()
