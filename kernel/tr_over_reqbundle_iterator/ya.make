LIBRARY()

OWNER(
    g:base
)

SRCS(
    break_word_accumulator.cpp
    reqbundle_buf_hits_provider.cpp
    tr_over_reqbundle_iterator.cpp
    tr_over_reqbundle_iterators_factory.cpp
    helpers.cpp
)

PEERDIR(
    kernel/lingboost
    kernel/reqbundle
    kernel/reqbundle_iterator
    kernel/doom/search_fetcher
    kernel/tr_over_reqbundle_iterator/proto
    library/cpp/pop_count
    ysite/yandex/posfilter
)

END()
