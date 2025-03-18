GO_LIBRARY()

OWNER(
    g:go-library
    g:solomon
)

SRCS(pusher.go)

END()

RECURSE(httppusher)
