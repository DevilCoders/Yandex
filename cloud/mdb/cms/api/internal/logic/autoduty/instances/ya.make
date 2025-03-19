GO_LIBRARY()

OWNER(g:mdb)

SRCS(autoduty_bin.go)

END()

RECURSE(
    duty
    opcontext
    steps
    workflow
)
