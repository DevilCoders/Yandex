GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    base.go
    check_is_master.go
    downtimes_create.go
    downtimes_remove.go
    helpers.go
    instance_to_fqdn_converter.go
    lock_acquire.go
    lock_release.go
    move.go
    wait_for_healthy.go
    whip_master.go
)

GO_XTEST_SRCS(
    base_test.go
    check_is_master_test.go
    instance_to_fqdn_converter_test.go
    lock_acquire_test.go
    lock_release_test.go
    move_test.go
    wait_for_healthy_test.go
    whip_master_test.go
)

END()

RECURSE(mocks)

RECURSE_FOR_TESTS(gotest)
