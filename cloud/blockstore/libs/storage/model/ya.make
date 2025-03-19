LIBRARY()

OWNER(g:cloud-nbs)

GENERATE_ENUM_SERIALIZATION(channel_data_kind.h)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/storage/core/libs/tablet/model
)

SRCS(
    channel_data_kind.cpp
    channel_permissions.cpp
)

END()
