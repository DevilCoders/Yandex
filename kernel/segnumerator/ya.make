LIBRARY()

OWNER(
    stanly
    velavokr
)

SRCS(
    detect_headers.cpp
    doc_node.cpp
    html_markers.rl6
    process_attributes.cpp
    process_markup.cpp
    process_tree.cpp
    readability.cpp
    segment.cpp
    structfinder.cpp
    segmentator.cpp
)

PEERDIR(
    contrib/libs/libidn
    kernel/hosts/owner
    kernel/segmentator
    kernel/segmentator/structs
    kernel/segnumerator/utils
    kernel/urlnorm
    library/cpp/html/entity
    library/cpp/html/face
    library/cpp/html/pdoc
    library/cpp/html/spec
    library/cpp/lcs
    library/cpp/numerator
    library/cpp/token
    library/cpp/tokenizer
    library/cpp/wordpos
    util/draft
)

END()
