OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/feature_flag_changed/resources/query.sql stg/yt/cdm/events/billing/feature_flag_changed/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/feature_flag_changed/resources/parameters.yaml stg/yt/cdm/events/billing/feature_flag_changed/resources/parameters.yaml
)

END()
