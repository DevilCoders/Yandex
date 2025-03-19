GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    backend.go
    format.go
    level.go
    logger.go
    memory.go
    multi.go
)

GO_TEST_SRCS(
    example_test.go
    format_test.go
    level_test.go
    log_test.go
    logger_test.go
    memory_test.go
    multi_test.go
)

IF (OS_LINUX)
    SRCS(
        log_nix.go
        syslog.go
    )
ENDIF()

IF (OS_DARWIN)
    SRCS(
        log_nix.go
        syslog.go
    )
ENDIF()

IF (OS_WINDOWS)
    SRCS(
        log_windows.go
        syslog_fallback.go
    )
ENDIF()

END()

RECURSE(
    gotest
)
