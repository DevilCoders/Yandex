import test_runner

"""
./download-logs.py --save-to-dir /tmp/local_cypress_tree --service morda --lower-filter-key y1000011061540484646 \
    --upper-filter-key y1000011061540484647 --date-from 20181025 --date-to 20181025 --yson-format text
"""


def test_squeeze_pool_morda(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["morda"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/morda/102313/20181025": 1
        },
    )
