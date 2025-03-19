OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/cdm/yt/antifraud/dm_ba_af_falsepositive/resources/query.sql cdm/yt/antifraud/dm_ba_af_falsepositive/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/cdm/yt/antifraud/dm_ba_af_falsepositive/resources/parameters.yaml cdm/yt/antifraud/dm_ba_af_falsepositive/resources/parameters.yaml
)

END()
