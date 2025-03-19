OWNER(g:cloud-nbs)

RECURSE(
    python
    recipes
)

RECURSE_FOR_TESTS(
    build-arcadia-test
    client
    fio
    fs_posix_compliance
    loadtest
    multiclient-rw-test
    service
)
