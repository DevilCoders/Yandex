GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    commands_client.go
    create_job_result_parameters.go
    create_job_result_responses.go
    create_shipment_parameters.go
    create_shipment_responses.go
    get_command_parameters.go
    get_command_responses.go
    get_commands_list_parameters.go
    get_commands_list_responses.go
    get_job_parameters.go
    get_job_responses.go
    get_job_result_parameters.go
    get_job_result_responses.go
    get_job_results_list_parameters.go
    get_job_results_list_responses.go
    get_jobs_list_parameters.go
    get_jobs_list_responses.go
    get_shipment_parameters.go
    get_shipment_responses.go
    get_shipments_list_parameters.go
    get_shipments_list_responses.go
)

END()
