PY2TEST()

OWNER(
    g:snippets
)

TEST_SRCS(
    test_additional_snippets.py
    test_additional_snippets_no_cuts.py
    test_additional_snippets_no_meta.py
    test_extended_snippet.py
    test_fact_snippet_dssm_factor.py
)

SIZE(SMALL)

TIMEOUT(60)

PEERDIR(
    kernel/snippets/base
)

DEPENDS(
    tools/snipmake/csnip
    tools/snipmake/dump_factsnip_factor
)

DATA(
    # snippet_contexts for extra snippets
    sbr://1969786305
    # snippet contexts for DSSM factor test:
    # 200_contexts_for_20_queries.tsv
    sbr://577073228
    # snippet contexts for extended snippet test:
    # 600_contexts_for_20_queries_desktop_touch_pad.tsv
    sbr://612079406
    # a DSSM for the fact snippet (a test fork):
    # RuFactSnippet.dssm
    sbr://577085095
)

REQUIREMENTS(ram:12)

END()
