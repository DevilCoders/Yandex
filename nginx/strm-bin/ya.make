# dummy comment #3 -- just to provoke CI to take the newest binary build
PROGRAM(nginx)

OWNER(
    toshik
    g:contrib
)

BUILD_ONLY_IF(LINUX)

NO_UTIL()

NGINX_MODULES(
    contrib/nginx/core
    contrib/nginx/core/src/http
    contrib/nginx/core/src/stream
    contrib/nginx/modules/lua-nginx-module
    contrib/nginx/modules/lua-upstream-nginx-module
    contrib/nginx/modules/echo-nginx-module
    contrib/nginx/modules/nginx-aws-auth-module
    contrib/nginx/modules/nginx-headers-more
    contrib/nginx/modules/nginx-pinba
    contrib/nginx/modules/nginx-upstream-fair
    contrib/nginx/modules/ngx_brotli
    contrib/nginx/modules/nginx-ip-tos-filter
    contrib/nginx/modules/nginx-ssl-ja3
    contrib/nginx/modules/ngx_http_proxy_connect_module
    contrib/nginx/modules/kaltura
    contrib/nginx/modules/nginx-rtmp-module
    nginx/modules/small
    nginx/modules/antirobot
    nginx/modules/sdch
    nginx/modules/geobase
    nginx/modules/icookie
    nginx/modules/nginx-fwmark
    nginx/modules/nginx-tcp-congestion
    nginx/modules/strm_packager
    nginx/modules/strm_lua_bundle
)

PEERDIR(
    strm/nginx_lua/adblock
    strm/nginx_lua/healthcheck
    strm/nginx_lua/uaas
)

END()

RECURSE_FOR_TESTS(
    tests
)
