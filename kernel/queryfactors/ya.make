OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    detectors.cpp
    lemmas_collector.cpp
    nwords_detector.cpp
    querybased.cpp
)

PEERDIR(
    kernel/indexer/direct_text
    kernel/indexer/face
    kernel/indexer/faceproc
    kernel/qtree/richrequest
    library/cpp/charset
)

END()
