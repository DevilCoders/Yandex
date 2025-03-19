GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cherrypy.go
    config.go
    saltutil.go
    state.go
    test.go
)

GO_TEST_SRCS(
    cherrypy_test.go
    saltutil_test.go
)

END()

RECURSE(gotest)
