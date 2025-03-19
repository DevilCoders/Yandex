OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/labels_maps/resources/query.sql ods/yt/billing/labels_maps/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/labels_maps/resources/parameters.yaml ods/yt/billing/labels_maps/resources/parameters.yaml
)

END()
