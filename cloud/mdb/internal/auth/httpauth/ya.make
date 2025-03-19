GO_LIBRARY()

OWNER(g:mdb)

SRCS(auth.go)

END()

RECURSE(
    blackboxauth
    combinedauth
    iamauth
    mocks
)
