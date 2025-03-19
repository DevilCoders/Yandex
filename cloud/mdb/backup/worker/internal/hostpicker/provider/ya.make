GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    config.go
    healthyhostpicker.go
    preferreplicahostpicker.go
)

GO_TEST_SRCS(preferredreplicahostpicker_test.go)

END()

RECURSE(gotest)
