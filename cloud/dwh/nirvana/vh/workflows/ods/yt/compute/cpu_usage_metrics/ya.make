OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/compute/cpu_usage_metrics/resources/query.sql ods/yt/compute/cpu_usage_metrics/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/compute/cpu_usage_metrics/resources/parameters.yaml ods/yt/compute/cpu_usage_metrics/resources/parameters.yaml
)

END()
