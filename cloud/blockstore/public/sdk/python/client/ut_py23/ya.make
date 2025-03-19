PY23_TEST()

OWNER(g:cloud-nbs)

SIZE(MEDIUM)
TIMEOUT(600)

INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/public/sdk/python/client/ut/ya.make.inc)

REQUIREMENTS(ram:12)

END()
