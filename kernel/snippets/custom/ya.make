LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/lemmer/alpha
    kernel/lemmer/core
    kernel/segmentator/structs
    kernel/snippets/algo
    kernel/snippets/archive
    kernel/snippets/archive/chooser
    kernel/snippets/archive/markup
    kernel/snippets/archive/unpacker
    kernel/snippets/archive/view
    kernel/snippets/archive/zone_checker
    kernel/snippets/calc_dssm
    kernel/snippets/config
    kernel/snippets/custom/data
    kernel/snippets/custom/hostnames_data
    kernel/snippets/custom/forums_handler
    kernel/snippets/custom/opengraph
    kernel/snippets/custom/preview_viewer
    kernel/snippets/custom/schemaorg_viewer
    kernel/snippets/custom/sea_json
    kernel/snippets/custom/trash_classifier
    kernel/snippets/cut
    kernel/snippets/delink
    kernel/snippets/entityclassify
    kernel/snippets/explain
    kernel/snippets/factors
    kernel/snippets/hits
    kernel/snippets/i18n
    kernel/snippets/iface/archive
    kernel/snippets/markers
    kernel/snippets/qtree
    kernel/snippets/read_helper
    kernel/snippets/replace
    kernel/snippets/schemaorg
    kernel/snippets/schemaorg/question
    kernel/snippets/sent_info
    kernel/snippets/sent_match
    kernel/snippets/simple_cmp
    kernel/snippets/simple_textproc/capital
    kernel/snippets/simple_textproc/decapital
    kernel/snippets/smartcut
    kernel/snippets/snip_builder
    kernel/snippets/snip_proto
    kernel/snippets/static_annotation
    kernel/snippets/strhl
    kernel/snippets/title_trigram
    kernel/snippets/titles
    kernel/snippets/titles/make_title
    kernel/snippets/uni_span_iter
    kernel/snippets/urlcut
    kernel/snippets/urlmenu/common
    kernel/snippets/urlmenu/cut
    kernel/snippets/urlmenu/dump
    kernel/snippets/weight
    kernel/snippets/wordstat
    kernel/tarc/docdescr
    kernel/tarc/iface
    library/cpp/charset
    library/cpp/containers/comptrie
    library/cpp/containers/comptrie/loader
    library/cpp/containers/dense_hash
    library/cpp/json
    library/cpp/langmask
    library/cpp/langs
    library/cpp/scheme
    library/cpp/stopwords
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
    library/cpp/tokenizer
    util/draft
)

SRCS(
    add_extended.cpp
    clicklike.cpp
    creativework.cpp
    dicacademic.cpp
    dmoz.cpp
    docsig.cpp
    eksisozluk.cpp
    encyc.cpp
    entity.cpp
    entity_viewer.cpp
    extended.cpp
    extended_length.cpp
    fact_snip.cpp
    fake_redirect.cpp
    forums.cpp
    generic_for_mobile.cpp
    hilitedurl.cpp
    isnip.cpp
    kinopoisk.cpp
    list_handler.cpp
    listsnip.cpp
    market.cpp
    mediawiki.cpp
    movie.cpp
    need_translate.cpp
    news.cpp
    preview.cpp
    productoffer.cpp
    question.cpp
    rating.cpp
    replacer_chooser.cpp
    replacer_weighter.cpp
    robot_dater.cpp
    robots_txt.cpp
    sahibinden.cpp
    simple_meta.cpp
    socnet.cpp
    software.cpp
    statannot.cpp
    static_extended.cpp
    struct_common.cpp
    suppress_cyr.cpp
    table_handler.cpp
    tablesnip.cpp
    translated_doc.cpp
    trash_annotation.cpp
    trash_viewer.cpp
    video_desc.cpp
    weighted_dmoz.cpp
    weighted_icatalog.cpp
    weighted_meta.cpp
    weighted_videodescr.cpp
    weighted_yaca.cpp
    yaca.cpp
    youtube_channel.cpp
)

END()
