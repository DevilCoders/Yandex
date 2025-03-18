LIBRARY()

OWNER(
    elric
    g:base
)

SRCS(
    finisher.h
    materialize.h
    resetter.h
    tagged.h
    masks.cpp
    dummy.cpp
)

PEERDIR(
    library/cpp/vec4
    library/cpp/containers/stack_array
)

END()
