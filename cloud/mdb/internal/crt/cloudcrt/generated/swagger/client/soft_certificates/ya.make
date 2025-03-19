GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    post_api_get_session_certificate_id_parameters.go
    post_api_get_session_certificate_id_responses.go
    post_api_list_session_certificate_parameters.go
    post_api_list_session_certificate_responses.go
    post_api_revoke_session_certificate_id_parameters.go
    post_api_revoke_session_certificate_id_responses.go
    post_api_session_certificate_parameters.go
    post_api_session_certificate_responses.go
    soft_certificates_client.go
)

END()
