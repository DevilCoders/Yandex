import test_runner


def test_squeeze_pool_intrasearch(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["intrasearch"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/intrasearch/0/20180205": 47
        },
    )
