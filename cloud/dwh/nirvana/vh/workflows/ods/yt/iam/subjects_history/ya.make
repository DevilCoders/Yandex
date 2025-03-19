OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/subjects_history/resources/query.sql ods/yt/iam/subjects_history/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/subjects_history/resources/parameters.yaml ods/yt/iam/subjects_history/resources/parameters.yaml
)

END()
