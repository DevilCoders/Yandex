LIBRARY()

OWNER(
    g:base
    sskvor
)

SRCS(
    generate.cpp
    validate.cpp
    write.cpp
)

PEERDIR(
    kernel/doom/item_storage
)

END()
