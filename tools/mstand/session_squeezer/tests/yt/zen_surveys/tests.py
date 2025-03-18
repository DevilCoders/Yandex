import test_runner

"""
./download-logs.py --save-to-dir /tmp/local_cypress_tree --service zen-surveys --lower-filter-key 17efb5bf845a302d294405028b7496bf \
--upper-filter-key 17efb5bf845a302d294405028b7496bg --date-from 20210716 --date-to 20210716  --yson-format text
"""


def test_squeeze_pool_surveys(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["zen-surveys"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/zen-surveys/387510/20210716": 11,
        },
    )
