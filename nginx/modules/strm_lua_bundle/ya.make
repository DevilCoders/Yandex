LIBRARY()

OWNER(g:strm-admins)

BUILD_ONLY_IF(LINUX)

NO_UTIL()

PEERDIR(
    contrib/nginx/core/src/http
    contrib/nginx/modules/lua-nginx-module
    contrib/libs/lua-cjson
)

SRCS(
    strm_lua_bundle.c
)

END()
