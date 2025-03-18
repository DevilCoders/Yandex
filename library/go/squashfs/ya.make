GO_LIBRARY()

OWNER(
    prime
    g:yt
    g:yt-go
)

SRCS(
    buffer.go
    reader.go
    writer.go
)

GO_TEST_SRCS(
    reader_test.go
    writer_test.go
)

END()

RECURSE(gotest)
