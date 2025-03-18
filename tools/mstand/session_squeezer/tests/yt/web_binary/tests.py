import test_runner

"""
./download-logs.py --save-to-dir /tmp/local_cypress_tree --service web --lower-filter-key y1000434551646129161 \
    --upper-filter-key y1000434551646129162 --date-from 20220301 --date-to 20220301 --yson-format text
"""


def test_squeeze_pool_web_using_binary(yt_stuff, squeeze_bin_file):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["web"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/web/0/20220701": 63,
        },
        squeeze_bin_file=squeeze_bin_file,
    )
