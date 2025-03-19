UNITTEST_FOR(kernel/facts/serp_parser)

OWNER(g:facts)

SRCS(
    extract_answer_ut.cpp
    extract_headline_ut.cpp
    extract_url_ut.cpp
)

RESOURCE(
    calories_fact.json calories_fact
    dict_fact.json dict_fact
    entity_fact.json entity_fact
    math_fact.json math_fact
    poetry_lover.json poetry_lover
    rich_fact.json rich_fact
    suggest_fact.json suggest_fact
    table_fact.json table_fact
    znatoki_fact.json znatoki_fact
    list_featured_snippet.json list_featured_snippet
)

END()
