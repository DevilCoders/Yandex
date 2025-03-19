GO_LIBRARY()

OWNER(g:mdb)

SRCS(manager.go)

GO_XTEST_SRCS(manager_test.go)

IF (OS_LINUX)
    SRCS(process_linux.go)
ENDIF()

IF (OS_DARWIN)
    SRCS(process_darwin.go)
ENDIF()

IF (OS_WINDOWS)
    SRCS(process_windows.go)
ENDIF()

END()

RECURSE(gotest)
