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
    # contrib/nginx/core/src/http/modules/perl  <~~~  disabled until it's clear how to use Perl in CI
    contrib/nginx/core/src/stream
)

END()

RECURSE_FOR_TESTS(
    tests
)
