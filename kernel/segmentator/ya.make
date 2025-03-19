LIBRARY()

OWNER(
    stanly
    velavokr
)

SRCS(
    main_content_impl.cpp
    main_header_impl.cpp
)

PEERDIR(
    kernel/matrixnet
    kernel/segmentator/structs
    library/cpp/charset
    library/cpp/packedtypes
    library/cpp/resource
    library/cpp/string_utils/base64
    library/cpp/string_utils/url
    library/cpp/wordpos
    util/draft
)

RESOURCE(
    main_content_matrixnet_model.info /main_content_matrixnet_model
)

END()
