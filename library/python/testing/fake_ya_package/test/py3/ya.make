PY3TEST()

OWNER(dmtrmonakhov)

DEPENDS(
    devtools/dummy_arcadia/hello_world
)
DATA(
    arcadia/devtools/ya/package/tests
    arcadia/devtools/ya/test/tests
)
PEERDIR(
    library/python/testing/fake_ya_package/test
)

END()
