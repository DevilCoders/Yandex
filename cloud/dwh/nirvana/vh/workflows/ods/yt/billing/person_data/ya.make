OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/person_data/resources/query.sql ods/yt/billing/person_data/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/person_data/resources/parameters.yaml ods/yt/billing/person_data/resources/parameters.yaml
)

END()
