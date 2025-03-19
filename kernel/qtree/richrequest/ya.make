OWNER(
    g:base
    g:wizard
)

LIBRARY()

ADDINCL(
    kernel/qtree/richrequest/markup
    kernel/qtree/richrequest/builder
    kernel/qtree/richrequest/serialization
)

SRCS(
    printrichnode.cpp
    pure_with_cached_word.cpp
    richtreebuilder.cpp
    builder/replacemultitoken.cpp
    builder/process_multitoken.cpp
    builder/subtreecreator.cpp
    builder/lemmatizer.cpp
    builder/postprocess.cpp
    nodesequence.cpp
    richtreeupdater.cpp
    richnode.cpp
    wordnode.cpp
    proxim.cpp
    init.cpp
    loadfreq.cpp
    lemmer_cache.cpp
    languagedisamb.cpp
    tokenlist.cpp
    verifytree.cpp
    helpers.cpp
    util.cpp
    serialization/text_format.cpp
    serialization/io.cpp
    serialization/serializer.cpp
    markup/markup.cpp
    markup/synonym.cpp
)

PEERDIR(
    kernel/fio
    kernel/idf
    kernel/keyinv/invkeypos
    kernel/lemmer
    kernel/qtree/compressor
    kernel/qtree/request
    kernel/qtree/richrequest/markup/wtypes
    kernel/qtree/richrequest/protos
    kernel/reqerror
    kernel/search_daemon_iface
    kernel/search_types
    library/cpp/charset
    library/cpp/json
    library/cpp/langmask/proto
    library/cpp/langmask/serialization
    library/cpp/protobuf/json
    library/cpp/protobuf/util
    library/cpp/stopwords
    library/cpp/streams/lz
    library/cpp/string_utils/base64
    library/cpp/string_utils/quote
    library/cpp/threading/mtp_tasks
    library/cpp/token
    library/cpp/token/serialization
    library/cpp/tokenclassifiers
    library/cpp/tokenizer
    library/cpp/unicode/punycode
    util/draft
    ysite/yandex/common
    ysite/yandex/pure
    ysite/yandex/reqdata
    library/cpp/deprecated/atomic
)

GENERATE_ENUM_SERIALIZATION(proxim.h)

GENERATE_ENUM_SERIALIZATION(range.h)

GENERATE_ENUM_SERIALIZATION(richnode.h)

END()
