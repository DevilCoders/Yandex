GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    notifications_client.go
    v2_escalations_stop_parameters.go
    v2_escalations_stop_responses.go
    v2_history_get_notification_rollups_parameters.go
    v2_history_get_notification_rollups_responses.go
    v2_history_get_notifications_parameters.go
    v2_history_get_notifications_responses.go
)

END()
