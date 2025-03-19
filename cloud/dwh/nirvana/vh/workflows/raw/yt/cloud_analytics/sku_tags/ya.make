OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/raw/yt/cloud_analytics/sku_tags/resources/query.sql raw/yt/cloud_analytics/sku_tags/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/raw/yt/cloud_analytics/sku_tags/resources/parameters.yaml raw/yt/cloud_analytics/sku_tags/resources/parameters.yaml
)

END()
