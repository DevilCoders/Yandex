import test_runner

"""
./download-logs.py --save-to-dir /tmp/local_cypress_tree --service news --lower-filter-key y100000261608888978 \
    --upper-filter-key y100000261608888979 --date-from 20210512 --date-to 20210512 --yson-format text
"""


def test_squeeze_pool_news(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["news"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/news/0/20210512": 27
        },
    )
