# -*- coding: utf-8 -*-

import mstand_utils.mstand_misc_helpers as mstand_umisc


class TestExtractHostFromUrl:
    def test_extract_host_raw_hostname(self):
        assert mstand_umisc.extract_host_from_url("") is None
        assert mstand_umisc.extract_host_from_url("yandex.ru") == "yandex.ru"
        assert mstand_umisc.extract_host_from_url("www.yandex.ru") == "www.yandex.ru"
        assert mstand_umisc.extract_host_from_url("Yandex.ru") == "yandex.ru"

        assert mstand_umisc.extract_host_from_url("www.yandex.ru", normalize=True) == "yandex.ru"

    def test_extract_host_raw_hostname_with_scheme(self):
        assert mstand_umisc.extract_host_from_url("http://yandex.ru") == "yandex.ru"
        assert mstand_umisc.extract_host_from_url("https://www.yandex.ru") == "www.yandex.ru"

    def test_extract_host_raw_hostname_with_params(self):
        assert mstand_umisc.extract_host_from_url("http://yandex.ru?text=yandex&lr=213") == "yandex.ru"
        assert mstand_umisc.extract_host_from_url("http://yandex.ru#suffix") == "yandex.ru"
        assert mstand_umisc.extract_host_from_url("https://www.yandex.ru/search?text=yandex") == "www.yandex.ru"

        assert mstand_umisc.extract_host_from_url("https://Www.yandex.ru/search?text=yandex", normalize=True) == "yandex.ru"

    def test_extract_host_bad_schema(self):
        assert mstand_umisc.extract_host_from_url("yandex.good/path://Yandex.bad") == "yandex.good"
        assert mstand_umisc.extract_host_from_url("yandex.good?param=value://Yandex.bad") == "yandex.good"
        assert mstand_umisc.extract_host_from_url("yandex.good#suffix://Yandex.bad") == "yandex.good"

    def test_normalize_with_port(self):
        assert mstand_umisc.extract_host_from_url("www.yandex.ru:443", normalize=True) == "yandex.ru"
        assert mstand_umisc.extract_host_from_url("www.yandex.ru:443", normalize=False) == "www.yandex.ru:443"
