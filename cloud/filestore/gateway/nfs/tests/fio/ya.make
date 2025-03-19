OWNER(g:cloud-nbs)

RECURSE_FOR_TESTS(
    # standard ganesha FSALs
    qemu-mem-test
    qemu-vfs-test

    # our FSAL over different storages
#     qemu-kikimr-nemesis-test
#     qemu-kikimr-test
#     qemu-local-test
)
