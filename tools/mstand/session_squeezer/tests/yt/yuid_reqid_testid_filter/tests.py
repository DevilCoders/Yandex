import test_runner

"""
./download-logs.py --save-to-dir /tmp/local_cypress_tree --service yuid-reqid-testid-filter --lower-filter-key \
    y10000021571493359 --upper-filter-key y10000021571493360 --date-from 20210421 --date-to 20210421 --yson-format text
"""


def test_squeeze_yuid_reqid_testid_filter(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["yuid-reqid-testid-filter"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/tmp/squeeze/testids/yuid-reqid-testid-filter/0/20210421": 3
        },
        use_filters=True,
        squeeze_path="//home/mstand/tmp/squeeze",
    )
