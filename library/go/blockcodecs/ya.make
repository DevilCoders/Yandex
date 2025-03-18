GO_LIBRARY()

OWNER(
    g:go-library
    prime
)

SRCS(
    codecs.go
    decoder.go
    encoder.go
    nop_codec.go
)

END()

RECURSE(
    all
    blockbrotli
    blocklz4
    blocksnappy
    blockzstd
    integration
)
