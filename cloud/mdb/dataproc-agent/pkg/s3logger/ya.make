GO_LIBRARY()

OWNER(
    g:mdb-dataproc
    g:mdb
)

SRCS(
    log_chunk.go
    pipe_processor.go
    s3logger.go
    s3uploader.go
)

GO_TEST_SRCS(pipe_processor_test.go)

END()

RECURSE(
    gotest
    mocks
)
