GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    downtimes_client.go
    v2_downtimes_get_downtimes_parameters.go
    v2_downtimes_get_downtimes_responses.go
    v2_downtimes_remove_downtimes_parameters.go
    v2_downtimes_remove_downtimes_responses.go
    v2_downtimes_set_downtimes_parameters.go
    v2_downtimes_set_downtimes_responses.go
)

END()
