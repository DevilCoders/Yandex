OWNER(g:cloud-nbs)

GO_LIBRARY()

SRCS(
    http_img_reader.go
    image_map.go
    image_map_reader.go
    image_reader.go
    raw_image_reader.go
    source.go
)

END()

RECURSE(
    common
    mapitem
    qcow2
)

RECURSE_FOR_TESTS(
    tests
)
