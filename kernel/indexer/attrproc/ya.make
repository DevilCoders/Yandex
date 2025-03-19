LIBRARY()

OWNER(g:base)

SRCS(
    url_transliterator.cpp
    attrportion.cpp
    attryndex.cpp
    catdata.cpp
    catwork.cpp
    catclosure.cpp
    attrproc.cpp
    attributer.cpp
    geodata.cpp
    multilangdata.cpp
)

PEERDIR(
    kernel/catfilter
    kernel/hosts/clons
    kernel/hosts/owner
    kernel/indexer/baseproc
    kernel/indexer/face
    kernel/indexer/faceproc
    kernel/indexer/lexical_decomposition
    kernel/keyinv/invkeypos
    kernel/langregion
    kernel/lemmer
    kernel/lemmer/translate
    kernel/lemmer/untranslit
    kernel/search_types
    kernel/translate
    library/cpp/charset
    library/cpp/containers/comptrie
    library/cpp/deprecated/fgood
    library/cpp/langmask
    library/cpp/microbdb
    library/cpp/mime/types
    library/cpp/on_disk/st_hash
    library/cpp/string_utils/url
    library/cpp/token
    ysite/yandex/common
    yweb/protos
    yweb/robot/dbscheeme
)

END()
