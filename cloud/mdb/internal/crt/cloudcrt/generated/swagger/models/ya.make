GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    db_acl_s_s_l.go
    db_cert_info.go
    db_soft_key.go
    requests_acl_s_s_l_req.go
    requests_error_response.go
    requests_issue_req.go
    requests_session_cert_sign_req.go
    requests_session_service_req.go
    server_list_acl_response.go
    server_list_certificates_response.go
    server_list_soft_certificates_response.go
)

END()
