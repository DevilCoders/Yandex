OWNER(g:cloud-kms)

GO_LIBRARY()

SRCS(
    default_keys_storage.go
    streaming_aead_common.go
    streaming_aead_decrypter.go
    streaming_aead_encrypter.go
    streaming_aead_range_decrypter.go
    streaming_aead_read_encrypter.go
    streamingcrypto.go
)

GO_TEST_SRCS(
    default_keys_storage_test.go
    streaming_aead_bench_test.go
    streaming_aead_test.go
)

END()

RECURSE(gotest)
RECURSE(proto/metadata)
