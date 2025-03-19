OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/became_trial/resources/query.sql stg/yt/cdm/events/billing/became_trial/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/became_trial/resources/parameters.yaml stg/yt/cdm/events/billing/became_trial/resources/parameters.yaml
)

END()
