OWNER(g:mdb)

IF (
    OS_DARWIN
    OR
    OS_LINUX
)
    RECURSE(lint)
ENDIF()

RECURSE(control_plane)
