LIBRARY()

    OWNER(alexvru)

    SRCS(
        wilson_event.h
        wilson_span.cpp
        wilson_span.h
        wilson_trace.h
        wilson_uploader.cpp
        wilson_uploader.h
    )

    PEERDIR(
        library/cpp/actors/core
        library/cpp/actors/protos
        library/cpp/actors/wilson/protos
    )

END()

RECURSE(
    protos
)
