import test_runner


def test_squeeze_pool_alice(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["alice"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/alice/144558/20190620": 1
        },
    )
