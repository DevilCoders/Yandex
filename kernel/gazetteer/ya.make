LIBRARY()

OWNER(
    dmitryno
    gotmanov
    g:wizard
)

PEERDIR(
    kernel/gazetteer/common
    kernel/gazetteer/proto
    kernel/gazetteer/protoparser
    kernel/lemmer
    kernel/lemmer/alpha
    kernel/lemmer/dictlib
    library/cpp/charset
    library/cpp/containers/comptrie
    library/cpp/deprecated/iter
    library/cpp/digest/md5
    library/cpp/langs
    library/cpp/packers
    library/cpp/protobuf/util
    library/cpp/token
    library/cpp/unicode/normalization
    library/cpp/deprecated/atomic
)

SRCS(
    gztparser.cpp
    articlepool.cpp
    gzttrie.cpp
    tokenize.cpp
    gazetteer.cpp
    generator.cpp
    worditerator.cpp
    articlefilter.cpp
    filterdata.cpp
    gztarticle.cpp
)

END()
