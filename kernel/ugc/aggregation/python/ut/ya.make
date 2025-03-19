OWNER(
    rufrozen
    g:ugc
)

PY2TEST()

TEST_SRCS(
    test_feedback.py
)

RESOURCE(
    kernel/ugc/aggregation/ut/data1/updates.json /data1/updates
    kernel/ugc/aggregation/ut/data1/result.json /data1/result
    kernel/ugc/aggregation/ut/data6/updates_feedback_override.json /data6/updates
    kernel/ugc/aggregation/ut/data7/updates.json /data7/updates
    kernel/ugc/aggregation/ut/data7/result.json /data7/result
)

PEERDIR(
    kernel/ugc/aggregation/python
    kernel/ugc/aggregation/proto
    library/python/resource
)

END()
