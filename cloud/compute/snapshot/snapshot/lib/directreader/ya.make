GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    imgreader.go
    map.go
    readat_readahead.go
    url_image_reader.go
)

GO_TEST_SRCS(
    imgreader_test.go
    map_test.go
    readat_readahead_test.go
    url_image_reader_test.go
)

END()

RECURSE(gotest)
