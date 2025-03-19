GO_LIBRARY()

OWNER(g:mdb)

SRCS(browser.go)

IF (OS_LINUX)
    SRCS(browser_linux.go)
ENDIF()

IF (OS_DARWIN)
    SRCS(browser_darwin.go)
ENDIF()

IF (OS_WINDOWS)
    SRCS(browser_windows.go)
ENDIF()

END()
