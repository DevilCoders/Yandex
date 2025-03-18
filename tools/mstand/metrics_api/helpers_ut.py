import metrics_api.helpers as mhelp


class TestMetricsApiHelpers:

    def test_get_yandex_service_by_url(self):
        test_urls = [
            ["https://Auto.ru/pskov/cars/used/", "auto"],
            ["https://yandex.ru/images/search?text=best%20tattoo%20old%20man&stype=image&lr=2&source=wiz", "images"],
            ["https://yandex.ru/video/search?text=chistor%20%D0%BF%D0%BE%D0%BA%D0%B0%D0%B7%D0%B0%D0%BB%20%D0%BB%D0%B8%D1%86%D0%BE", "video"],
            ["https://rabota.yandex.ru/search?currency=RUR&from=wizard.region&utm_source=wizard&rid=213", "rabota"],
            ["https://market.yandex.ru/search?text=jade%208 8&lr=2&clid=545", "market"],
            ["https://market.yandex.fake/search?text=jade%20888&lr=2&clid=545", ""],
            ["https://www.Avito.ru/pskov/nedvizhimost", ""],
            ["", ""],
        ]
        for url, good_result in test_urls:
            result = mhelp.get_yandex_service_by_url(url)
            assert result == good_result
