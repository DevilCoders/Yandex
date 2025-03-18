PY3_LIBRARY()

OWNER(
    g:mstand
)

PY_SRCS(
    NAMESPACE sample_metrics.online
    __init__.py
    bucket_metric.py
    clicks_without_requests.py
    online_lamps.py
    online_metric_helpers.py
    online_test_metrics.py
    spu.py
    spu_v2.py
    sspu.py
    total_number_of_clicks.py
    total_number_of_requests.py
    web_abandonment.py
    web_no_top3_clicks.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
