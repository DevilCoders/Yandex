OWNER(g:cloud-nbs)

RECURSE_FOR_TESTS(
    # standard ganesha FSALs
    service-mem-test
    service-vfs-test

    # our FSAL over different storages
    service-kikimr-test
    service-local-test
)
