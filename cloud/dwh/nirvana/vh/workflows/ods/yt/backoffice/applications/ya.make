OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/backoffice/applications/resources/query.sql ods/yt/backoffice/applications/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/backoffice/applications/resources/parameters.yaml ods/yt/backoffice/applications/resources/parameters.yaml
)

END()
