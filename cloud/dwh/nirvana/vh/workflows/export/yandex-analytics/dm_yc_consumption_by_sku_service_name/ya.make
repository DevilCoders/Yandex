OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/export/yandex-analytics/dm_yc_consumption_by_sku_service_name/resources/query.sql export/yandex-analytics/dm_yc_consumption_by_sku_service_name/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/export/yandex-analytics/dm_yc_consumption_by_sku_service_name/resources/parameters.yaml export/yandex-analytics/dm_yc_consumption_by_sku_service_name/resources/parameters.yaml
)

END()
