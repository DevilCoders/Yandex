LIBRARY()

OWNER(sobols)

SRCS(
    cast.h
    copy.h
    declare_fields.h
    empty.cpp
    impl/tags.h
    map.h
    mapping.h
    maybe.h
    set.h
    string.h
    type_traits.h
    unordered_map.h
    unordered_set.h
    vector.h
    writer.h
)

PEERDIR(
    contrib/libs/mms
)

END()
