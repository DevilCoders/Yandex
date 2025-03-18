LIBRARY()

OWNER(
    toshik
    g:contrib
)

BUILD_ONLY_IF(LINUX)

NO_UTIL()

CFLAGS(-Wno-unused-parameter)

PEERDIR(
    contrib/nginx/core/src/http
    contrib/libs/zlib
    library/c/tvmauth
)

ADDINCL(contrib/nginx/core/objs)

SRCS(
    ngx_addtag_exe_filter_module.c
    ngx_http_auth_sign_module.cpp
    ngx_http_cookie_obf_module.c
    #ngx_http_eblob_module.c
    ngx_http_flv_filter_module.c
    #ngx_http_json_log_module.c
    ngx_http_request_id_module.c
    ngx_http_tcp_syn_options_module.c
    ngx_http_tskvlog_module.c
    ngx_http_fake_connection.c
    ngx_http_tvm2_module.c
    ngx_http_yandex_vars.c
    utils.c
)

END()
