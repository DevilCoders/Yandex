PY3TEST()

OWNER(g:cloud-nbs)

SIZE(LARGE)
TIMEOUT(60)

REQUIREMENTS(
    container:2273379275
)

TAG(ya:fat ya:force_sandbox ya:privileged)

# requires root
IF (NOT AUTOCHECK)
    TAG(ya:manual)
ENDIF()

TEST_SRCS(
    test.py
)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/nfs-ganesha-vfs.inc)

END()
