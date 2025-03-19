OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/tiered_sku_counters/resources/query.sql ods/yt/billing/tiered_sku_counters/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/tiered_sku_counters/resources/parameters.yaml ods/yt/billing/tiered_sku_counters/resources/parameters.yaml
)

END()
