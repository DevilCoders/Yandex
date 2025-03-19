OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/first_service_consumption/resources/query.sql stg/yt/cdm/events/billing/first_service_consumption/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/first_service_consumption/resources/parameters.yaml stg/yt/cdm/events/billing/first_service_consumption/resources/parameters.yaml
)

END()
