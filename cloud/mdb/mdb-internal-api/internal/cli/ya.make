GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    commander.go
    config.go
)

END()

RECURSE(
    cmds
    logic
)
