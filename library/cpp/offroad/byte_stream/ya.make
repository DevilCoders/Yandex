LIBRARY()

OWNER(
    elric
    lagrunge
    sankear
    g:base
)

SRCS(
    byte_input_stream.h
    byte_output_stream.h
    output_stream_base.h
    raw_output_stream.h
)

PEERDIR(
    library/cpp/offroad/codec
    library/cpp/offroad/keyinv
    library/cpp/vec4
)

END()
