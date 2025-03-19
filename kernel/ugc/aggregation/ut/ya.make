UNITTEST_FOR(kernel/ugc/aggregation)

OWNER(g:ugc)

PEERDIR(
    library/cpp/resource
)

SRCS(
    feedback_ut.cpp
    feedback_v2_ut.cpp
)

RESOURCE(
    data1/updates.json                      /data1/updates
    data1/result.json                       /data1/result
    data1/result_v2.json                    /data1/result_v2
    data2/direct_order.json                 /data2/direct_order
    data2/direct_order_result.json          /data2/direct_order_result
    data2/direct_order_result_v2.json       /data2/direct_order_result_v2
    data2/reverse_order.json                /data2/reverse_order
    data2/reverse_order_result.json         /data2/reverse_order_result
    data2/reverse_order_result_v2.json      /data2/reverse_order_result_v2
    data2/order_test1.json                  /data2/order_test1
    data2/order_test1_result.json           /data2/order_test1_result
    data2/order_test2.json                  /data2/order_test2
    data2/order_test2_result.json           /data2/order_test2_result
    data3/updates.json                      /data3/updates
    data3/result.json                       /data3/result
    data4/updates.json                      /data4/updates
    data4/result.json                       /data4/result
    data4/result_v2.json                    /data4/result_v2
    data5/updates.json                      /data5/updates
    data5/result.json                       /data5/result
    data5/result_v2.json                    /data5/result_v2
    data6/updates_feedback_override.json    /data6/updates_feedback_override
    data6/updates_feedback_rating.json      /data6/updates_feedback_rating
    data6/updates_feedback_review.json      /data6/updates_feedback_review
    data6/result_feedback_override.json     /data6/result_feedback_override
    data6/result_feedback_override_v2.json  /data6/result_feedback_override_v2
    data6/result_feedback_rating.json       /data6/result_feedback_rating
    data6/result_feedback_rating_v2.json    /data6/result_feedback_rating_v2
    data6/result_feedback_review.json       /data6/result_feedback_review
    data6/result_feedback_review_v2.json    /data6/result_feedback_review_v2
    data7/updates.json                      /data7/updates
    data7/result.json                       /data7/result
    data7/result_v2.json                    /data7/result_v2
    data8/updates.json                      /data8/updates
    data8/result.json                       /data8/result
    data8/result_v2.json                    /data8/result_v2
    data9/updates.json                      /data9/updates
    data9/result.json                       /data9/result
    data9/result_v2.json                    /data9/result_v2
    data10/updates.json                     /data10/updates
    data10/result.json                      /data10/result
    data10/result_v2.json                   /data10/result_v2
)

END()
