GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    delete_task_parameters.go
    delete_task_responses.go
    get_request_status_parameters.go
    get_request_status_responses.go
    list_unhandled_management_requests_parameters.go
    list_unhandled_management_requests_responses.go
    register_request_to_manage_nodes_parameters.go
    register_request_to_manage_nodes_responses.go
    tasks_client.go
)

END()
