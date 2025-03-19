OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/compute_disks_usage_by_billing/resources/query.sql ods/yt/compute_disks_usage_by_billing/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/compute_disks_usage_by_billing/resources/parameters.yaml ods/yt/compute_disks_usage_by_billing/resources/parameters.yaml
)

END()
