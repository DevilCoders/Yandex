GO_PROGRAM(salt-operator-manager)

SRCS(main.go)

END()

RECURSE(
    api
    cmd
    controllers
    internal
    pkg
)
