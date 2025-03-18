GO_LIBRARY()

OWNER(
    prime
    g:geotargeting
    g:go-library
)

SRCS(geobase.go)

IF (CGO_ENABLED)
    SRCS(cgo.go)
ELSE()
    SRCS(stub.go)
ENDIF()

GO_XTEST_SRCS(
    example_test.go
    geobase_test.go
)

END()

RECURSE(gotest)

IF (CGO_ENABLED)
    RECURSE(internal)
ENDIF()
