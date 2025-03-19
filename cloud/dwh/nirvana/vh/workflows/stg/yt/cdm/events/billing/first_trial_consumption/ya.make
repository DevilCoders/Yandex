OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/first_trial_consumption/resources/create_query.sql stg/yt/cdm/events/billing/first_trial_consumption/resources/create_query.sql
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/first_trial_consumption/resources/parameters.yaml stg/yt/cdm/events/billing/first_trial_consumption/resources/parameters.yaml
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/first_trial_consumption/resources/update_query.sql stg/yt/cdm/events/billing/first_trial_consumption/resources/update_query.sql
)

END()
