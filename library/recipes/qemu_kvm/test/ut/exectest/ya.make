OWNER(
    dmitko
    g:yatool
)

EXECTEST()

RUN(
    NAME is_root
    /usr/bin/id
    STDOUT ${TEST_OUT_ROOT}/stdout.log
    CANONIZE_LOCALLY ${TEST_OUT_ROOT}/stdout.log
)

RUN(
    NAME ip_addr
    /bin/ip a l
)

SET(QEMU_SSH_USER root)
INCLUDE(${ARCADIA_ROOT}/library/recipes/qemu_kvm/recipe.inc)
END()
