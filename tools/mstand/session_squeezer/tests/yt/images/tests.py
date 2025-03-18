import test_runner


def test_squeeze_pool_images(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["images"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/images/103686/20181031": 447
        },
    )
