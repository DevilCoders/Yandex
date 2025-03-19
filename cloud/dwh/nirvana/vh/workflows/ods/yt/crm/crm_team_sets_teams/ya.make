OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_team_sets_teams/resources/query.sql ods/yt/crm/crm_team_sets_teams/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_team_sets_teams/resources/parameters.yaml ods/yt/crm/crm_team_sets_teams/resources/parameters.yaml
)

END()
