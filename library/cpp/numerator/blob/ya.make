LIBRARY()

OWNER(stanly)

SRCS(
    numeratorevents.cpp
    numserializer.cpp
)

PEERDIR(
    library/cpp/html/face
    library/cpp/html/face/blob
    library/cpp/html/storage
    library/cpp/html/zoneconf
    library/cpp/numerator
    library/cpp/packedtypes
)

END()
