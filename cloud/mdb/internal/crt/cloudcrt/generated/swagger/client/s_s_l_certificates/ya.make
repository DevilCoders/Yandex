GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    delete_api_certificate_id_parameters.go
    delete_api_certificate_id_responses.go
    get_api_certificate_id_download_pem_parameters.go
    get_api_certificate_id_download_pem_responses.go
    get_api_certificate_id_download_pfx_parameters.go
    get_api_certificate_id_download_pfx_responses.go
    get_api_certificate_id_parameters.go
    get_api_certificate_id_responses.go
    get_api_certificate_parameters.go
    get_api_certificate_responses.go
    post_api_certificate_parameters.go
    post_api_certificate_responses.go
    ssl_certificates_client.go
)

END()
