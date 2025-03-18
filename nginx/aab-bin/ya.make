# dummy comment #3 -- just to provoke CI to take the newest binary build
PROGRAM(nginx)

OWNER(
    ashaposhnikov
    g:antiadblock
    g:contrib
)

BUILD_ONLY_IF(LINUX)

NO_UTIL()

NGINX_MODULES(
    contrib/nginx/core
    contrib/nginx/core/src/http
    contrib/nginx/core/src/stream
    contrib/nginx/core/src/http/modules/perl
    contrib/nginx/modules/lua-nginx-module
    contrib/nginx/modules/echo-nginx-module
    contrib/nginx/modules/nginx-dav-ext-module
    contrib/nginx/modules/nginx-headers-more
    contrib/nginx/modules/nginx-pinba
    contrib/nginx/modules/nginx-upstream-fair
    contrib/nginx/modules/ngx_brotli
    contrib/nginx/modules/nginx-ip-tos-filter
    contrib/nginx/modules/nginx-ssl-ja3
    nginx/modules/small
    nginx/modules/filesub
    nginx/modules/force_gunzip
)

END()
