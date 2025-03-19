LIBRARY()

OWNER(g:remorph)

SRCS(
    impl/article.cpp
    impl/articles.cpp
    impl/blob.cpp
    impl/compound_field.cpp
    impl/compound_fields.cpp
    impl/fact.cpp
    impl/factory.cpp
    impl/facts.cpp
    impl/field.cpp
    impl/field_container.cpp
    impl/fields.cpp
    impl/info.cpp
    impl/processor.cpp
    impl/range.cpp
    impl/results.cpp
    impl/sentence.cpp
    impl/solutions.cpp
    impl/token.cpp
    impl/tokens.cpp
)

PEERDIR(
    contrib/libs/protobuf
    kernel/gazetteer
    kernel/remorph/facts
    kernel/remorph/info
    kernel/remorph/input
    kernel/remorph/text
    kernel/remorph/tokenizer
    library/cpp/json
    library/cpp/protobuf/json
    library/cpp/solve_ambig
    library/cpp/token
)

END()

