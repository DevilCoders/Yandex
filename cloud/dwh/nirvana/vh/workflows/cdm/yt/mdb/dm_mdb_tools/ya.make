OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/cdm/yt/mdb/dm_mdb_tools/resources/parameters.yaml cdm/yt/mdb/dm_mdb_tools/resources/parameters.yaml
    cloud/dwh/nirvana/vh/workflows/cdm/yt/mdb/dm_mdb_tools/resources/query.sql cdm/yt/mdb/dm_mdb_tools/resources/query.sql
)

END()
