GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    create_minion_parameters.go
    create_minion_responses.go
    delete_minion_parameters.go
    delete_minion_responses.go
    get_minion_master_parameters.go
    get_minion_master_responses.go
    get_minion_parameters.go
    get_minion_responses.go
    get_minions_list_parameters.go
    get_minions_list_responses.go
    minions_client.go
    register_minion_parameters.go
    register_minion_responses.go
    unregister_minion_parameters.go
    unregister_minion_responses.go
    upsert_minion_parameters.go
    upsert_minion_responses.go
)

END()
