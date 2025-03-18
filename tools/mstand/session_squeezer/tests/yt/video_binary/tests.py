import test_runner

"""
./download-logs.py --service video --date-from 20220525 --date-to 20220525 \
    --lower-filter-key y1000002351652559852 --upper-filter-key y1000002351652559853 --save-to-dir /tmp/local_cypress_tree
"""


def test_squeeze_pool_video_using_binary(yt_stuff, squeeze_bin_file):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["video"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/video/126283/20220525": 41
        },
        squeeze_bin_file=squeeze_bin_file,
    )
