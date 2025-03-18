GO_LIBRARY()

OWNER(
    prime
    g:passport_infra
    g:go-library
)

IF (CGO_ENABLED)
    PEERDIR(library/cpp/tvmauth/client)

    SRCS(
        CGO_EXPORT
        tvm.cpp
    )

    CGO_SRCS(
        client.go
        logger.go
    )
ELSE()
    SRCS(
        stub.go
    )
ENDIF()

SRCS(
    doc.go
    types.go
)

GO_XTEST_SRCS(client_example_test.go)

END()

IF (CGO_ENABLED)
    RECURSE_FOR_TESTS(
        apitest
        gotest
        tiroletest
        tooltest
    )
ENDIF()
