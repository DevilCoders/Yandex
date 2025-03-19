OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/budgeter_logs/resources/query.sql ods/yt/billing/budgeter_logs/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/budgeter_logs/resources/parameters.yaml ods/yt/billing/budgeter_logs/resources/parameters.yaml
)

END()
