GO_LIBRARY()

OWNER(g:mdb)

SRCS(scheduler.go)

END()

RECURSE(
    generic
    sequential
)
