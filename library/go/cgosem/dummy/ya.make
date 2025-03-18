GO_LIBRARY()

OWNER(
    prime
    g:go-library
)

IF (CGO_ENABLED)
    PEERDIR(util)

    SRCS(dummy.cpp)

    CGO_SRCS(dummy.go)
ELSE()
    SRCS(stub.go)
ENDIF()

END()
