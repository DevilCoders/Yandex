LIBRARY()

#!!!
OWNER(
    ddoarn
    a-romanov
    pg
    gulin
    dcherednik
    g:yamr
    g:rtmr
)

PEERDIR(
    contrib/libs/crcutil
)

SRCS(
    crc32c.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
