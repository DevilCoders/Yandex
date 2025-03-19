GO_PROGRAM()

OWNER(g:cloud-nbs)

DATA(arcadia/cloud/compute/snapshot/config.toml)

ENV(TEST_CONFIG_PATH=cloud/compute/snapshot/config.toml)

TAG(
    ya:fat
    ya:privileged
    ya:force_sandbox
    ya:norestart
    ya:sys_info
    ya:noretries
)

SIZE(LARGE)

REQUIREMENTS(
    container:773239891
    # xenial
    cpu:all
    dns:dns64
)

SRCS(
    flags.go
    main.go
)

GO_TEST_SRCS(snapshot_client_test.go)

END()

RECURSE(gotest)
