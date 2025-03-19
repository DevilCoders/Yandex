OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/cloud_analytics/sku_tags/resources/query.sql ods/yt/cloud_analytics/sku_tags/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/cloud_analytics/sku_tags/resources/parameters.yaml ods/yt/cloud_analytics/sku_tags/resources/parameters.yaml
)

END()
