import test_runner

"""
./download-logs.py --save-to-dir /tmp/local_cypress_tree --service web-surveys --lower-filter-key \
    y4569220481572892618 --upper-filter-key y4569220481572892619 --date-from 20210826 --date-to 20210826 --yson-format text
"""


def test_squeeze_pool_web_surveys(yt_stuff, squeeze_bin_file):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["web-surveys"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/web-surveys/409479/20210826": 5,
        },
        squeeze_bin_file=squeeze_bin_file,
    )
