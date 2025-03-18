import test_runner


def test_squeeze_pool_ott_impressions(yt_stuff):
    test_runner.run_test(
        client=yt_stuff.get_yt_client(),
        services=["ott-impressions"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            "//home/mstand/squeeze/testids/ott-impressions/"
            "media_billing_client_validate_promocode_experiment.validatePromoCode/20210322": 37
        },
    )
