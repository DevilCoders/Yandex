GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    downtimes_get.go
    downtimes_set.go
    juggler.go
)

END()

RECURSE(generated)
