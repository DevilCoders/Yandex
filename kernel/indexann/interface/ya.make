LIBRARY()

OWNER(
    g:indexann
)

SRCS(
    visitor.h
    reader.cpp
    writer.cpp
)

PEERDIR(
    kernel/indexann/hit
)

END()
