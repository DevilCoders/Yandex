LIBRARY()

OWNER(
    yurikiselev
    g:snippets
)

PEERDIR(
    kernel/snippets/idl
    library/cpp/json
)

SRCS(
    raw_preview_fillers.h
    raw_preview_fillers.cpp
    sea_json.h
    sea_json.cpp
)

END()
