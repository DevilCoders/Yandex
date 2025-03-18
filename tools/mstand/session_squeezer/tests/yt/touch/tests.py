import test_runner

"""
./download-logs.py --save-to-dir /tmp/local_cypress_tree --service touch \
    --lower-filter-key "uu/000010af71efd51e935a446e9eafa7e4" --upper-filter-key "uu/000010af71efd51e935a446e9eafa7e5" \
    --date-from 20181026 --date-to 20181026 --yson-format text
"""


def test_squeeze_pool_touch(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["touch"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/tmp/squeeze/testids/touch/100376/20181026": 27
        },
        squeeze_path="//home/mstand/tmp/squeeze",
    )


def test_squeeze_pool_web_touch_extended(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["web-touch-extended"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/tmp/squeeze/testids/web-touch-extended/100376/20181026": 2
        },
        squeeze_path="//home/mstand/tmp/squeeze",
    )
