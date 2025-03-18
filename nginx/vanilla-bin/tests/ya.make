PY3TEST()

OWNER(
    toshik
    g:contrib
)

PEERDIR(
    contrib/nginx/tests
)

DATA(
    arcadia/contrib/nginx/tests/lib
    # arcadia/contrib/nginx/core/src/http/modules/perl/generated  <~~~  disabled until it's clear how to use Perl in CI
)

DEPENDS(
    nginx/vanilla-bin
)

TEST_SRCS(
    test_tap.py
)

TIMEOUT(600)
SIZE(MEDIUM)

FORK_SUBTESTS()

END()
