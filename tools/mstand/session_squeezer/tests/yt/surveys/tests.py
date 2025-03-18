import test_runner

"""
./download-logs.py --save-to-dir /tmp/local_cypress_tree --service surveys --lower-filter-key y100001691524789265 \
--upper-filter-key y100001691524789266 --date-from 20220109 --date-to 20220109  --yson-format text
"""


def test_squeeze_pool_surveys(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["surveys"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/surveys/478822/20220109": 17,
        },
    )
