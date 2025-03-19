LIBRARY()

OWNER(g:facts)

SRCS(
    fconsole_fact_rule.cpp
    rule_application.cpp
    rule_parsers.cpp
    substring_match_normalization.cpp
)

PEERDIR(
    kernel/facts/word_set_match
    search/idl
)

END()

RECURSE_FOR_TESTS(
    ut
)
