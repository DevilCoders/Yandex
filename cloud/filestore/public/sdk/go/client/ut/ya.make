GO_TEST_FOR(cloud/filestore/public/sdk/go/client)

OWNER(g:cloud-nbs)

SIZE(MEDIUM)
TIMEOUT(600)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/service-null.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/vhost.inc)

END()
