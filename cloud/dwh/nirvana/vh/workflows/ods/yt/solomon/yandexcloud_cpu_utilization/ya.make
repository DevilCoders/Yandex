OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/solomon/yandexcloud_cpu_utilization/resources/query.sql ods/yt/solomon/yandexcloud_cpu_utilization/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/solomon/yandexcloud_cpu_utilization/resources/parameters.yaml ods/yt/solomon/yandexcloud_cpu_utilization/resources/parameters.yaml
)

END()
