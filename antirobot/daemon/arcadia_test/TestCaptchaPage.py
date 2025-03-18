import base64
import gzip
import json
import re
import urllib.parse
import urllib.request

import brotli

import pytest

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test import util
from antirobot.daemon.arcadia_test.util import (
    captcha_page,
    Fullreq,
)

TEST_REQ_ID = "927980946-936738376"


class TestCaptchaPage(AntirobotTestSuite):
    options = {
        "Proxycaptcha_image": "1",
    }

    captcha_args = ["--generate", "correct", "--check", "success"]
    fury_args = ["--button-check", "fail"]

    def test_captcha_image(self):
        page = captcha_page.HtmlCaptchaPage(self, "yandex.ru")

        assert page.get_ret_path()
        assert page.get_key()
        assert len(page.get_img_urls()) == 1

    @pytest.mark.parametrize(
        "host, language",
        [
            ("yandex.ru", "ru"),
            ("wspm-dev2.yandex.ru:8080", "ru"),
            ("yandex.ua", "ru"),
            ("wspm-dev2.yandex.ua:8080", "ru"),
            ("yandex.by", "ru"),
            ("wspm-dev2.yandex.by:8080", "ru"),
            ("yandex.kz", "kz"),
            ("wspm-dev2.yandex.kz:8080", "kz"),
            ("yandex.uz", "uz"),
            ("m.yandex.ru", "ru"),
            ("m.yandex.ua", "ru"),
            ("m.yandex.by", "ru"),
            ("m.yandex.kz", "kz"),
            ("m.yandex.uz", "uz"),
            ("yandex.com", "en"),
            ("yandex.eu", "en"),
            ("yandex.com:4444", "en"),
            ("yandex.net", "en"),
            ("afraud-dev.search.yandex.net:8080", "en"),
            ("m.yandex.com", "en"),
            ("m.yandex.eu", "en"),
            ("m.yandex.net", "en"),
            ("yandex.com.tr", "tr"),
            ("somehost.yandex.com.tr:8888", "tr"),
            ("m.yandex.com.tr", "tr"),
            ("eda.yandex", "ru"),
            ("m.eda.yandex", "ru"),
        ],
    )
    def test_localization_by_tld(self, host, language):
        self.check_localization(host, None, None, language)

    @pytest.mark.parametrize(
        "cookie, language",
        [
            (util.MY_COOKIE_ENG, "en"),
            (util.MY_COOKIE_RUS, "ru"),
            (util.MY_COOKIE_UKR, "ru"),
            (util.MY_COOKIE_BEL, "ru"),
            (util.MY_COOKIE_TUR, "tr"),
            (util.MY_COOKIE_KAZ, "kz"),
            (util.MY_COOKIE_UZB, "uz"),
            (util.MY_COOKIE_TAT, "ru"),
        ],
    )
    def test_localization_by_my_cookie(self, cookie, language):
        if cookie == util.MY_COOKIE_ENG:
            host = "yandex.ru"
        else:
            host = "yandex.com"

        self.check_localization(host, cookie, None, language)

    def test_localization_by_accept_language(self):
        host = "yandex.org"
        accept_language = "ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7"
        language = "ru"
        self.check_localization(host, None, accept_language, language)

    def check_localization(self, host, cookie, accept_language, language):
        samples_with_check_other_langs = {
            "ru": [
                "Введите текст с картинки",
                "Отправить",
                ">Ой!</title>",
            ],
            "en": [
                "Enter the code from the image",
                "Submit",
                ">Oops!</title>",
            ],
            "tr": [
                "Görselden metin girin",
                "Gönder",
                ">Tüh!</title>",
            ],
            "kz": [
                "Суреттегі кодты енгізіңіз",
                "Жіберу",
                ">Ой!</title>",
            ],
            "uz": [
                "Rasmdagi matnni kiriting",
                "Yuborish",
                ">Oh!</title>",
            ],
        }
        samples_without_check_other_langs = {
            "ru": [
                '<meta data-react-helmet="true" property="og:image" content="https://yastatic.net/s3/home-static/_/37/37a02b5dc7a51abac55d8a5b6c865f0e.png">',
                '<meta data-react-helmet="true" property="og:title" content="Яндекс">',
                '<meta data-react-helmet="true" property="og:description" content="Найдётся всё">',
            ],
            "en": [
                '<meta data-react-helmet="true" property="og:image" content="https://yastatic.net/s3/home-static/_/90/9034470dfcb0bea0db29a281007b8a38.png">',
                '<meta data-react-helmet="true" property="og:title" content="Yandex">',
                '<meta data-react-helmet="true" property="og:description" content="Finds everything">',
            ],
            "tr": [
                '<meta data-react-helmet="true" property="og:image" content="https://yastatic.net/s3/home-static/_/90/9034470dfcb0bea0db29a281007b8a38.png">',
                '<meta data-react-helmet="true" property="og:title" content="Yandex">',
                '<meta data-react-helmet="true" property="og:description" content="Her şey bulunur">',
            ],
            "kz": [

                '<meta data-react-helmet="true" property="og:image" content="https://yastatic.net/s3/home-static/_/90/9034470dfcb0bea0db29a281007b8a38.png">',
                '<meta data-react-helmet="true" property="og:title" content="Яндекс">',
                '<meta data-react-helmet="true" property="og:description" content="Бәрі табылады">',
            ],
            "uz": [
                '<meta data-react-helmet="true" property="og:image" content="https://yastatic.net/s3/home-static/_/90/9034470dfcb0bea0db29a281007b8a38.png">',
                '<meta data-react-helmet="true" property="og:title" content="Yandex">',
                '<meta data-react-helmet="true" property="og:description" content="Hammasi topiladi">',
            ],
        }
        for lang, str_list in samples_without_check_other_langs.items():
            str_list.extend([
                'prefix="og: http://ogp.me/ns#"',
                "onsubmit=\"document.getElementById('advanced-captcha-form')",
            ])

        headers = {}

        if cookie is not None:
            headers["Cookie"] = "my=" + cookie

        if accept_language is not None:
            headers["accept-language"] = accept_language

        page = captcha_page.HtmlCaptchaPage(
            self, host, headers=headers
        ).get_content()

        for samples, check_other in [(samples_with_check_other_langs, False), (samples_without_check_other_langs, False)]:
            for lang, words in samples.items():
                for word in words:
                    if lang == language:
                        assert word in page
                    elif check_other and word in page:
                        print(word)
                        index = page.index(word)
                        context = page[index - 20 : index + len(word) + 20]
                        assert (
                            False
                        ), f"Page mustn't contain '{word}' which found at {context}"

    @pytest.mark.parametrize(
        "captcha_cls, content_type",
        [
            (captcha_page.XmlCaptchaPage, "text/xml"),
            (captcha_page.HtmlCaptchaPage, "text/html"),
            (captcha_page.JsonCaptchaPage, "application/json"),
            (captcha_page.JsonPCaptchaPage, "application/javascript"),
        ],
    )
    def test_content_type(self, captcha_cls, content_type):
        page = captcha_cls(self, "yandex.ru")

        assert page.get_content_type() == content_type

    @pytest.mark.parametrize(
        "check_captcha_cls, content_type",
        [
            (captcha_page.HtmlCheckCaptcha, None),
            (captcha_page.JsonCheckCaptcha, "application/json"),
            (captcha_page.JsonPCheckCaptcha, "application/javascript"),
            (captcha_page.HtmlCheckCaptcha, None),
        ],
    )
    def test_check_captcha_content_type(self, check_captcha_cls, content_type):
        host = "yandex.ru"

        page = captcha_page.HtmlCaptchaPage(self, host)
        check_page = check_captcha_cls(self, host, page.get_key(), "fake_rep", page.get_ret_path())
        assert check_page.get_content_type() == content_type

    @pytest.mark.parametrize(
        "captcha_class, host, expected_language",
        [
            (captcha_page.HtmlCaptchaPage, "yandex.ru", "ru"),
            (captcha_page.HtmlCaptchaPage, "yandex.ua", "ru"),
            (captcha_page.HtmlCaptchaPage, "yandex.by", "ru"),
            (captcha_page.HtmlCaptchaPage, "yandex.kz", "ru"),
            (captcha_page.HtmlCaptchaPage, "yandex.uz", "ru"),
            (captcha_page.HtmlCaptchaPage, "m.yandex.ru", "ru"),
            (captcha_page.HtmlCaptchaPage, "m.yandex.ua", "ru"),
            (captcha_page.HtmlCaptchaPage, "m.yandex.by", "ru"),
            (captcha_page.HtmlCaptchaPage, "m.yandex.kz", "ru"),
            (captcha_page.HtmlCaptchaPage, "m.yandex.uz", "ru"),
            (captcha_page.HtmlCaptchaPage, "yandex.com", "en"),
            (captcha_page.HtmlCaptchaPage, "yandex.eu", "en"),
            (captcha_page.HtmlCaptchaPage, "yandex.net", "en"),
            (captcha_page.HtmlCaptchaPage, "yandex.lt", "en"),
            (captcha_page.HtmlCaptchaPage, "m.yandex.com", "en"),
            (captcha_page.HtmlCaptchaPage, "m.yandex.eu", "en"),
            (captcha_page.HtmlCaptchaPage, "yandex.com.tr", "tr"),
            (captcha_page.JsonCaptchaPage, "yandex.ru", "ru"),
            (captcha_page.XmlCaptchaPage, "xmlsearch.yandex.ru", "ru"),
            (captcha_page.JsonCaptchaPage, "yandex.net", "en"),
            (captcha_page.XmlCaptchaPage, "xmlsearch.yandex.com", "en"),
            (captcha_page.XmlCaptchaPage, "xmlsearch.yandex.eu", "en"),
            (captcha_page.XmlCaptchaPage, "xmlsearch.yandex.com.tr", "tr"),
            (captcha_page.XmlImagesCaptchaPage, "yandex.ru", "ru"),
            (captcha_page.XmlImagesCaptchaPage, "yandex.by", "ru"),
            (captcha_page.HtmlCaptchaPage, "eda.yandex", "ru"),
        ],
    )
    def test_check_captcha_voice_lang(
        self, captcha_class, host, expected_language
    ):
        page = captcha_class(self, host).get_content()
        image_urls = captcha_page.CAPTCHA_IMG_RE.findall(page)

        assert len(image_urls) > 0

        for url in image_urls:
            actualLanguage = captcha_page.GetLanguageFromImageUrl(url, self)
            assert actualLanguage == expected_language, f"{captcha_class} {host}"

    @pytest.mark.parametrize(
        "captcha_class, host, expected_captcha_type",
        [
            (captcha_page.HtmlCaptchaPage, "yandex.ru", "txt_v1"),
            (captcha_page.HtmlCaptchaPage, "yandex.ua", "txt_v1"),
            (captcha_page.HtmlCaptchaPage, "yandex.by", "txt_v1"),
            (captcha_page.HtmlCaptchaPage, "yandex.kz", "txt_v1"),
            (captcha_page.HtmlCaptchaPage, "yandex.uz", "txt_v1"),
            (captcha_page.HtmlCaptchaPage, "m.yandex.ru", "txt_v1"),
            (captcha_page.HtmlCaptchaPage, "m.yandex.ua", "txt_v1"),
            (captcha_page.HtmlCaptchaPage, "m.yandex.by", "txt_v1"),
            (captcha_page.HtmlCaptchaPage, "m.yandex.kz", "txt_v1"),
            (captcha_page.HtmlCaptchaPage, "m.yandex.uz", "txt_v1"),
            (captcha_page.HtmlCaptchaPage, "yandex.com", "txt_v1_en"),
            (captcha_page.HtmlCaptchaPage, "yandex.eu", "txt_v1_en"),
            (captcha_page.HtmlCaptchaPage, "yandex.net", "txt_v1_en"),
            (captcha_page.HtmlCaptchaPage, "yandex.lt", "txt_v1_en"),
            (captcha_page.HtmlCaptchaPage, "m.yandex.com", "txt_v1_en"),
            (captcha_page.HtmlCaptchaPage, "m.yandex.eu", "txt_v1_en"),
            (captcha_page.HtmlCaptchaPage, "yandex.com.tr", "txt_v1_en"),
            (captcha_page.JsonCaptchaPage, "yandex.ru", "ocr"),
            (captcha_page.XmlCaptchaPage, "xmlsearch.yandex.ru", "ocr"),
            (captcha_page.XmlNonBrandedPartnerCaptchaPage, "xmlsearch.yandex.ru", "txt_v1"),
            (captcha_page.XmlImagesCaptchaPage, "yandex.ru", "ocr"),
            (captcha_page.XmlImagesCaptchaPage, "yandex.by", "ocr"),
            (captcha_page.XmlImagesNonBrandedPartnerCaptchaPage, "yandex.ru", "txt_v1"),
            (captcha_page.XmlImagesNonBrandedPartnerCaptchaPage, "yandex.by", "txt_v1"),
            (captcha_page.HtmlCaptchaPage, "eda.yandex", "txt_v1"),
        ],
    )
    def test_check_captcha_type(self, captcha_class, host, expected_captcha_type):
        page = captcha_class(self, host).get_content()
        image_urls = captcha_page.CAPTCHA_IMG_RE.findall(page)

        assert len(image_urls) > 0

        for url in image_urls:
            actual_captcha_type = captcha_page.GetCaptchaTypeFromImageUrl(url, self)
            assert (
                actual_captcha_type == expected_captcha_type
            ), f"{captcha_class} {host}"

    @pytest.mark.parametrize(
        "captcha_class, host, additional_query, expected_rambler",
        [
            (captcha_page.HtmlCaptchaPage, "yandex.ru", "", False),
            (captcha_page.XmlNonBrandedPartnerCaptchaPage, "xmlsearch.yandex.ru", "", False),
            (captcha_page.XmlImagesNonBrandedPartnerCaptchaPage, "yandex.ru", "", False),
            (captcha_page.HtmlCaptchaPage, "yandex.ru", "&captcha_domain=rambler", False),  # Здесь идёт запрос /showcaptcha, поэтому параметр &captcha_domain=rambler не пробрасывается
            (captcha_page.XmlNonBrandedPartnerCaptchaPage, "xmlsearch.yandex.ru", "&captcha_domain=rambler", True),
            (captcha_page.XmlImagesNonBrandedPartnerCaptchaPage, "yandex.ru", "&captcha_domain=rambler", True),
        ]
    )
    def test_check_parthner_proxy(self, captcha_class, host, additional_query, expected_rambler):
        page = captcha_class(self, host, additional_query=additional_query).get_content()
        image_urls = captcha_page.CAPTCHA_IMG_RE.findall(page)

        assert len(image_urls) > 0

        for url in image_urls:
            image_location = self.send_request(Fullreq(url)).headers['Location']
            if expected_rambler:
                assert image_location.startswith("https://nova.rambler.ru/ext_image?")
                assert url.startswith("http://nova.rambler.ru/captchaimg?")
            else:
                assert image_location.startswith("http://u.captcha.yandex.net/image?")
                assert url.startswith("http://yandex.ru/captchaimg?")

    @pytest.mark.parametrize(
        "host, service, expected_captcha_type, is_deprecated_ajax",
        [
            ("kinopoisk.ru",        "kinopoisk", "ocr",    False),
            ("m.kinopoisk.ru",      "kinopoisk", "ocr",    False),
            ("auto.ru",             "autoru",    "ocr",    False),
            ("m.auto.ru",           "autoru",    "ocr",    False),
            ("webmaster.yandex.ru", "webmaster", "txt_v1", True),  # NOTE: для теста нужен хотя бы один is_deprecated_ajax=True
            ("eda.yandex",          "eda",       "txt_v1", False),
        ],
    )
    def test_check_ajax_services(self, host, service, expected_captcha_type, is_deprecated_ajax):
        page = self.send_request(
            util.Fullreq(
                f"http://{host}/search?text=Капчу%21&ajax=1",
                headers={
                    "X-Antirobot-MayBanFor-Y": 1,
                    "X-Antirobot-Service-Y": service,
                }
            )
        ).read().decode()
        resp = json.loads(page)

        assert resp["type"] == "captcha"
        if is_deprecated_ajax:
            actual_captcha_type = captcha_page.GetCaptchaTypeFromImageUrl(resp["captcha"]["img-url"], self)
            assert actual_captcha_type == expected_captcha_type
        else:
            assert resp["captcha"]["img-url"] == ""

    def test_check_images_ajax(self):
        request_url = (
            "https://yandex.ru/images/search?format=json&text=%s"
            % urllib.request.quote("Капчу!")
        )
        page = self.send_request(util.Fullreq(request_url)).read().decode()
        image_urls = captcha_page.CAPTCHA_IMG_RE.findall(page)
        expected_captcha_type = "ocr"

        assert len(image_urls) > 0

        for url in image_urls:
            actual_captcha_type = captcha_page.GetCaptchaTypeFromImageUrl(url, self)
            assert actual_captcha_type == expected_captcha_type

    @pytest.mark.parametrize(
        "host, service",
        [
            ("yandex.ru", "web"),
            ("kinopoisk.ru", "kinopoisk"),
            ("m.kinopoisk.ru", "kinopoisk"),
            ("auto.ru", "autoru"),
            ("m.auto.ru", "autoru"),
            ("webmaster.yandex.ru", "webmaster"),
        ],
    )
    def test_check_ajax_ret_path_in_initial_request(self, host, service):
        request_url = "http://" + host + "/search?text=Капчу%21&ajax=1"
        headers = {
            "X-Antirobot-MayBanFor-Y": 1,
            "X-Antirobot-Service-Y": service,
            "X-Retpath-Y": "http://yandex.ru/test_from_initial",
        }

        captcha_response = self.send_request(util.Fullreq(request_url, headers=headers)).read().decode()
        retpath = captcha_page.GetRetpathFromJsonResponse(captcha_response)

        assert retpath.startswith("aHR0cDovL3lhbmRleC5ydS90ZXN0X2Zyb21faW5pdGlhbA,,_")

    @pytest.mark.parametrize(
        "header_value, valid_expected",
        [
            ("some_shit", False),
            ("http://yandex.ru/not_shit", True),
        ],
    )
    def test_invalid_retpath_in_json_captcha(self, header_value, valid_expected):
        request_url = "http://yandex.ru/search?text=Капчу%21&ajax=1"
        headers = {
            "X-Antirobot-MayBanFor-Y": 1,
            "X-Antirobot-Service-Y": "web",
            "X-Retpath-Y": header_value,
        }

        resp = self.send_request(util.Fullreq(request_url, headers=headers))
        if valid_expected:
            retpath = captcha_page.GetRetpathFromJsonResponse(resp.read().decode())
            assert retpath.startswith(f"{header_value}_")
        else:
            assert resp.getcode() == 400

    def test_check_metrika_in_desktop_captcha_page(self):
        request_url = "https://yandex.ru/search?text=%s" % urllib.request.quote(
            "Капчу!"
        )
        response = self.send_request(util.Fullreq(request_url))

        assert util.IsCaptchaRedirect(
            response
        ), f"Request sent to url '{request_url}' is not redirected to captcha"

        captcha_location = response.info()["Location"]
        page = self.send_request(util.Fullreq(captcha_location)).read().decode()

        assert 'mc.yandex.ru/watch/10630330' in page, f"Page in '{request_url}' does not have metrika script"

    def test_check_metrika_in_mobile_captcha_page(self):
        request_url = (
            "https://yandex.ru/search/touch?text=%s"
            % urllib.request.quote("Капчу!")
        )
        response = self.send_request(util.Fullreq(request_url))

        assert util.IsCaptchaRedirect(
            response
        ), f"Request sent to url '{request_url}' is not redirected to captcha"

        captcha_location = response.info()["Location"]
        page = self.send_request(util.Fullreq(captcha_location)).read().decode()

        assert 'mc.yandex.ru/watch/10630330' in page, f"Page in '{request_url}' does not have metrika script"

    def test_captcha_time_metrics(self):
        request_url = "https://yandex.ru/search?text=%s" % urllib.request.quote(
            "Капчу!"
        )
        headers = {"X-Antirobot-MayBanFor-Y": 1}
        response = self.send_request(util.Fullreq(request_url, headers=headers))

        assert util.IsCaptchaRedirect(
            response
        ), f"Request sent to url '{request_url}' is not redirected to captcha"

        captcha_location = response.info()["Location"]

        admin_requests = 0
        all_requests_before = self.antirobot.get_metric("http_server.time_10s_deee")
        captcha_requests_before = self.antirobot.get_metric("captcha_time_10s_deee")
        admin_requests += 2

        self.send_request(util.Fullreq(captcha_location, headers=headers)).read()

        all_requests_after = self.antirobot.get_metric("http_server.time_10s_deee")
        captcha_requests_after = self.antirobot.get_metric("captcha_time_10s_deee")

        assert all_requests_before == all_requests_after - admin_requests
        assert captcha_requests_after > captcha_requests_before

    def test_captcha_time_metrics_ajax(self):
        admin_requests = 0
        all_requests_before = self.antirobot.get_metric("http_server.time_10s_deee")
        captcha_requests_before = self.antirobot.get_metric("captcha_time_10s_deee")
        admin_requests += 2

        request_url = (
            "https://yandex.ru/search?text=%s&ajax=1"
            % urllib.request.quote("Капчу!")
        )
        headers = {"X-Antirobot-MayBanFor-Y": 1}
        self.send_request(util.Fullreq(request_url, headers=headers))

        all_requests_after = self.antirobot.get_metric("http_server.time_10s_deee")
        captcha_requests_after = self.antirobot.get_metric("captcha_time_10s_deee")

        assert all_requests_before == all_requests_after - admin_requests
        assert captcha_requests_after > captcha_requests_before

    @pytest.mark.parametrize(
        "host, doc, service, add_service_header",
        [
            ("yandex.ru", "/search", "web", False),
            ("auto.ru", "/search", "autoru", True),
            ("kinopoisk.ru", "/search", "kinopoisk", True),
            ("yandex.ru", "/images/search", "img", False),
            ("beru.ru", "/search", "marketblue", True),
        ],
    )
    def test_check_image_url_service_parameter(
        self, host, doc, service, add_service_header
    ):
        headers = {"X-Antirobot-MayBanFor-Y": 1}
        if add_service_header:
            headers.update({"X-Antirobot-Service-Y": service})

        page = captcha_page.HtmlCaptchaPage(self, host, doc, headers)

        for imageUrl in page.get_img_urls():
            encoded_real_url = imageUrl.split("?")[1].split("_")[0]
            real_url = base64.b64decode(encoded_real_url.replace(",", "="))
            parsed_url = urllib.parse.urlparse(real_url)

            assert urllib.parse.parse_qs(parsed_url.query)[b"service"] == \
                [service.encode()]

    def test_correct_captcha_image_url(self):
        page = captcha_page.HtmlCaptchaPage(self, "yandex.ru", "/search")
        url = page.get_img_urls()[0]

        resp = self.send_request(util.Fullreq(url))

        assert resp.getcode() == 302
        assert (
            resp.info()["Location"]
            == "http://u.captcha.yandex.net/image?key=10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF&type=txt_v1&vtype=ru"
        )

    def test_correct_unique_key(self):
        self.unified_agent.pop_event_logs()
        page = captcha_page.HtmlCaptchaPage(self, "yandex.ru", "/search")
        strKey = str(self.unified_agent.get_last_event_in_daemon_logs()["unique_key"])

        events = self.unified_agent.get_event_logs(["TCaptchaShow"])

        assert len(events) != 0

        for event in events:
            assert event.Event.Header.UniqueKey == strKey

        assert f'unique_key:"{strKey}"' in page.get_content()

    @pytest.mark.parametrize(
        "static_path, content_type",
        [
            ("/captcha.min.css", "text/css"),
            ("/captcha.ie.min.css", "text/css"),
        ],
    )
    def test_static(self, static_path, content_type):
        url = "https://yandex.ru" + static_path
        resp = self.send_request(util.Fullreq(url))
        assert resp.getcode() == 200
        assert resp.info()["X-ForwardToUser-Y"] == "1"
        assert resp.info()["Content-Type"] == content_type

    @pytest.mark.parametrize(
        "static_url, user_agent",
        [
            ("/captcha_smart.\\w+.min.css", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 YaBrowser/22.1.0.2500 Yowser/2.5 Safari/537.36"),  # noqa: E501
            ("/captcha_smart.\\w+.min.js", ""),
            ("/captcha_smart_error.\\w+.min.js", ""),

            ("/captcha_smart.min.css", "Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 10.0; WOW64; Trident/7.0; .NET4.0C; .NET4.0E; .NET CLR 2.0.50727; .NET CLR 3.0.30729; .NET CLR 3.5.30729; Tablet PC 2.0)"),  # noqa: E501
            ("/captcha_smart.min.css", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; Trident/7.0; .NET4.0C; .NET4.0E; .NET CLR 2.0.50727; .NET CLR 3.0.30729; .NET CLR 3.5.30729; rv:11.0) like Gecko"),  # noqa: E501
        ],
    )
    def test_has_static_urls(self, static_url, user_agent):
        request_url = "https://yandex.ru/search?text=%s" % urllib.request.quote("Капчу!")
        headers = {
            "X-Antirobot-MayBanFor-Y": 1,
            "X-Req-Id": TEST_REQ_ID,
            "User-Agent": user_agent,
        }

        response = self.send_request(util.Fullreq(request_url, headers=headers))
        assert util.IsCaptchaRedirect(response)
        captcha_location = response.info()["Location"]
        page = self.send_request(
            util.Fullreq(captcha_location, headers=headers)
        ).read().decode()

        if static_url.endswith(".js"):
            assert re.search("src=\"{}\\?k=\\d+\"".format(static_url), page)
        else:
            assert re.search("href=\"{}\\?k=\\d+\"".format(static_url), page)

    @pytest.mark.parametrize("encoding", ["identity", "br", "gzip"])
    def test_compressed_greed(self, encoding):
        response = self.send_fullreq(
            "http://yandex.ru/captchapgrd",
            headers={"Accept-Encoding": encoding},
        )

        if encoding != "identity":
            assert response.headers["Content-Encoding"] == encoding

        assert re.match("public, max-age=\\d+, immutable", response.headers["Cache-Control"])

        body = response.read()

        if encoding == "br":
            body = brotli.decompress(body)
        elif encoding == "gzip":
            body = gzip.decompress(body)

        assert b"PGreed" in body
