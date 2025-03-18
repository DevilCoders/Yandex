OWNER(
    a-bocharov
    g:snippets
)

PY2TEST()

TEST_SRCS(
    segmentator_apply_test.py
    segmentator_split_factors_test.py
    segmentator_merge_factors_test.py
    segmentator_annotate_factors_test.py
)

DEPENDS(tools/snipmake/steam/page_factors/cpp_factors/segmentator_tool)



END()
