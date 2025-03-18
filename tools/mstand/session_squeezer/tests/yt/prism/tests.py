import test_runner


def test_squeeze_pool_prism(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["prism"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/prism/333339/20210318": 1
        },
    )
