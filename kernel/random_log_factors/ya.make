LIBRARY()

OWNER(
    alejes
    ptanusha
    g:neural-search
)

PEERDIR(
    library/cpp/packedtypes
    library/cpp/string_utils/base64
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(factor_types.h)

END()

NEED_CHECK()
