GO_LIBRARY()

OWNER(g:mdb)

SRCS(commander.go)

END()

RECURSE(
    browser
    config
    federation
    iam
    server
    token
    tracker
)
