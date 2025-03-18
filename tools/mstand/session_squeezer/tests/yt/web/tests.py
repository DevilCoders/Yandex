import test_runner

"""
./download-logs.py --save-to-dir /tmp/local_cypress_tree --service web --lower-filter-key y9554996871489345503 \
    --upper-filter-key y9554996871489345504 --date-from 20170319 --date-to 20170319 --yson-format text
./download-logs.py --save-to-dir /tmp/local_cypress_tree --service web --lower-filter-key y1000019861531142767 \
    --upper-filter-key y1000019861531142768 --date-from 20190531 --date-to 20190531 --yson-format text
./download-logs.py --save-to-dir /tmp/local_cypress_tree --service web --lower-filter-key y1000053521616506653 \
    --upper-filter-key y1000053521616506654 --date-from 20210402 --date-to 20210402 --yson-format text
./download-logs.py --save-to-dir /tmp/local_cypress_tree --service web --lower-filter-key y1000053521616506653 \
    --upper-filter-key y1000053521616506654 --date-from 20210403 --date-to 20210403 --yson-format text
./download-logs.py --save-to-dir /tmp/local_cypress_tree --service web --lower-filter-key y1000053521616506653 \
    --upper-filter-key y1000053521616506654 --date-from 20210404 --date-to 20210404 --yson-format text
"""


# noinspection PyClassHasNoInit
class TestSqueezeWeb:
    def test_squeeze_pool_web(self, yt_stuff):
        test_runner.run_test(
            client=yt_stuff.get_yt_client(),
            services=["web"],
            pool_path=test_runner.get_pool_path(__file__),
            path_to_row_count={
                "//home/mstand/tmp/squeeze/testids/web/143353/20190531": 10
            },
            squeeze_path="//home/mstand/tmp/squeeze",
        )


# noinspection PyClassHasNoInit
class TestSqueezeWebHistory:
    def test_squeeze_history_pool_web(self, yt_stuff):
        test_runner.run_test(
            client=yt_stuff.get_yt_client(),
            services=["web"],
            history=2,
            pool_path=test_runner.get_pool_path(__file__, filename="history_pool.json"),
            path_to_row_count={
                "//home/mstand/tmp/squeeze/testids/web/311060/20210404": 13,
                "//home/mstand/tmp/squeeze/history/web/311060/20210404_20210404/20210402": 14,
                "//home/mstand/tmp/squeeze/history/web/311060/20210404_20210404/20210403": 9,
            },
            squeeze_path="//home/mstand/tmp/squeeze",
        )


# noinspection PyClassHasNoInit
class TestSqueezeWebFuture:
    def test_squeeze_future_pool_web(self, yt_stuff):
        test_runner.run_test(
            client=yt_stuff.get_yt_client(),
            services=["web"],
            future=2,
            pool_path=test_runner.get_pool_path(__file__, filename="future_pool.json"),
            path_to_row_count={
                "//home/mstand/tmp/squeeze/testids/web/311060/20210402": 14,
                "//home/mstand/tmp/squeeze/history/web/311060/20210402_20210402/20210403": 9,
                "//home/mstand/tmp/squeeze/history/web/311060/20210402_20210402/20210404": 13,
            },
            squeeze_path="//home/mstand/tmp/squeeze",
        )


# noinspection PyClassHasNoInit
class TestSqueezeWebExtended:
    def test_squeeze_pool_web_desktop_extended(self, yt_stuff):
        test_runner.run_test(
            client=yt_stuff.get_yt_client(),
            services=["web-desktop-extended"],
            pool_path=test_runner.get_pool_path(__file__),
            path_to_row_count={
                "//home/mstand/tmp/squeeze/testids/web-desktop-extended/143353/20190531": 3
            },
            squeeze_path="//home/mstand/tmp/squeeze",
        )


# @pytest.skip("Fix later or remove.")
# def test_squeeze_web_unicode_decode_error_skip(yt_stuff):
#     with pytest.raises(Exception, match=r"Empty tables are not allowed.*"):
#         test_runner.run_test(
#             client=yt_stuff.get_yt_client(),
#             services=["web", "touch", "web-desktop-extended", "web-touch-extended"],
#             pool_path=test_runner.get_pool_path(__file__, filename="unicode_decode_error_pool.json"),
#             path_to_row_count={
#                 "//home/mstand/squeeze/testids/touch/40368/20170319": 0,
#                 "//home/mstand/squeeze/testids/touch/40369/20170319": 0,
#                 "//home/mstand/squeeze/testids/web-desktop-extended/40368/20170319": 0,
#                 "//home/mstand/squeeze/testids/web-desktop-extended/40369/20170319": 0,
#                 "//home/mstand/squeeze/testids/web-touch-extended/40368/20170319": 0,
#                 "//home/mstand/squeeze/testids/web-touch-extended/40369/20170319": 0,
#                 "//home/mstand/squeeze/testids/web/40368/20170319": 0,
#                 "//home/mstand/squeeze/testids/web/40369/20170319": 0,
#             },
#             use_filters=True,
#         )
