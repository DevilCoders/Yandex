OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/committed_use_discounts/resources/query.sql ods/yt/billing/committed_use_discounts/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/committed_use_discounts/resources/parameters.yaml ods/yt/billing/committed_use_discounts/resources/parameters.yaml
)

END()
