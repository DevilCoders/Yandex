GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    dashboards_client.go
    v2_dashboards_get_dashboards_parameters.go
    v2_dashboards_get_dashboards_responses.go
    v2_dashboards_remove_dashboard_parameters.go
    v2_dashboards_remove_dashboard_responses.go
    v2_dashboards_set_dashboard_parameters.go
    v2_dashboards_set_dashboard_responses.go
)

END()
