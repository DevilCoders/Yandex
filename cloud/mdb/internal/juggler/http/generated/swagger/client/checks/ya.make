GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    checks_client.go
    v2_checks_get_checks_config_parameters.go
    v2_checks_get_checks_config_responses.go
    v2_checks_get_checks_count_parameters.go
    v2_checks_get_checks_count_responses.go
    v2_checks_get_checks_state_parameters.go
    v2_checks_get_checks_state_responses.go
    v2_history_get_check_history_parameters.go
    v2_history_get_check_history_responses.go
    v2_history_get_check_history_rollups_parameters.go
    v2_history_get_check_history_rollups_responses.go
    v2_history_get_check_snapshot_parameters.go
    v2_history_get_check_snapshot_responses.go
    v2_namespaces_migrate_checks_parameters.go
    v2_namespaces_migrate_checks_responses.go
)

END()
