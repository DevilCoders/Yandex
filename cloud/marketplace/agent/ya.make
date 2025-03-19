OWNER(g:cloud-marketplace)

RECURSE(pkg)

IF (OS_WINDOWS)
    RECURSE(windows)
ENDIF()
