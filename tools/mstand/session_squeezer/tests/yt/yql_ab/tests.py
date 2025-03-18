import test_runner


def test_squeeze_pool_yql_ab(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["yql-ab"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/yql-ab/343729/20210405": 1
        },
    )
