import test_runner


def test_squeeze_pool_watch_log(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["watchlog"],
        pool_path=test_runner.get_pool_path(tests_path=__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/watchlog/138023:20190501/20190501": 10
        },
    )


def test_squeeze_pool_watch_log_unicode_decode_error_skip(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["watchlog"],
        pool_path=test_runner.get_pool_path(tests_path=__file__, filename="unicode_decode_error_pool.json"),
        path_to_row_count={
            "//home/mstand/squeeze/testids/watchlog/0:20200909/20200909": 10
        },
    )
