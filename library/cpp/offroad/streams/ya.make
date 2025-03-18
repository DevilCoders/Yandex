LIBRARY()

OWNER(
    elric
    sankear
    g:base
)

SRCS(
    bit_input.h
    bit_output.h
    vec_input.h
    vec_memory_input.h
    vec_output.h
)

PEERDIR(
    library/cpp/vec4
    library/cpp/offroad/offset
    library/cpp/offroad/utility
)

END()
