LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/i18n
    kernel/snippets/schemaorg/question
    kernel/snippets/schemaorg/proto
    kernel/snippets/sent_info
    kernel/snippets/sent_match
    library/cpp/langs
    library/cpp/string_utils/base64
    library/cpp/string_utils/url
)

SRCS(
    creativework.cpp
    movie.cpp
    sozluk_comments.cpp
    product_offer.cpp
    videoobj.cpp
    rating.cpp
    schemaorg_parse.cpp
    schemaorg_serializer.cpp
    schemaorg_traversal.cpp
    software.cpp
    youtube_channel.h
)

END()
