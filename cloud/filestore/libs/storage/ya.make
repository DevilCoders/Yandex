OWNER(g:cloud-nbs)

RECURSE(
    api
    core
    init
    model
    service
    ss_proxy
    tablet
    tablet/model
    tablet/protos
    tablet_proxy
)

RECURSE_FOR_TESTS(
    testlib
)
