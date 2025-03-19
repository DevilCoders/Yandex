OWNER(yurakura)

LIBRARY()

SRCS(
    default.cpp
)

FROM_SANDBOX(107988474 OUT geodb.serialized)

ARCHIVE_ASM(
    NAME KERNEL_GEODB_DEFAULT_GEODB_SERIALIZED
    geodb.serialized
)

PEERDIR(
    kernel/geodb
    library/cpp/archive
)

END()
