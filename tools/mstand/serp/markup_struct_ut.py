from serp import SerpMarkupInfo
from serp import SerpUrlsInfo
from serp import ResMarkupInfo
from serp import ResUrlInfo


# noinspection PyClassHasNoInit
class TestSerpMarkupInfo:
    def test_serialize_regular(self):
        pmi1 = ResMarkupInfo(pos=1, scales={})
        pmi2 = ResMarkupInfo(pos=2, scales={})
        markup_info = SerpMarkupInfo(qid=100500, res_markups=[pmi1, pmi2])
        ser_markup = markup_info.serialize()
        assert "qid" in ser_markup
        assert "markups" in ser_markup
        assert "serp_data" not in ser_markup

    def test_serialize_empty(self):
        markup_info = SerpMarkupInfo(qid=100500)
        ser_markup = markup_info.serialize()
        assert "qid" in ser_markup
        assert "markup" not in ser_markup
        assert "serp_data" not in ser_markup

    def test_serialize_wizard(self):
        rmi = ResMarkupInfo(pos=1, is_wizard=True)
        rmi_data = rmi.serialize()
        assert "isWizard" in rmi_data
        rmi_deser = ResMarkupInfo.deserialize(rmi_data)
        assert rmi_deser.is_wizard is True

    def test_deserialize_regular(self):
        ser_qmi = {
            "qid": 100500,
            "markups": [
                {
                    "pos": 1,
                    "scales": {}
                }
            ]
        }
        qmi = SerpMarkupInfo.deserialize(ser_qmi)
        assert qmi.qid == 100500
        assert len(qmi.res_markups) == 1

    def test_deserialize_empty(self):
        ser_markup = {
            "qid": 100500
        }
        markup_info = SerpMarkupInfo.deserialize(ser_markup)
        assert markup_info.qid == 100500
        assert len(markup_info.res_markups) == 0
        assert len(markup_info.serp_data) == 0


# noinspection PyClassHasNoInit
class TestSerpUrlsInfo:
    def test_serialize_regular(self):
        url_info1 = ResUrlInfo(pos=1, url="http://yandex.ru")
        url_info2 = ResUrlInfo(pos=2, url="http://google.com")
        qmi = SerpUrlsInfo(qid=100500, res_urls=[url_info1, url_info2])
        ser_qmi = qmi.serialize()
        assert "qid" in ser_qmi
        assert "urls" in ser_qmi

    def test_serialize_empty(self):
        urls_info = SerpUrlsInfo(qid=100500)
        ser_urls = urls_info.serialize()
        assert "qid" in ser_urls
        assert "urls" not in ser_urls

    def test_deserialize_regular(self):
        ser_urls = {
            "qid": 100500,
            "urls": [
                {
                    "pos": 1,
                    "url": "http://yandex.ru"
                }
            ]
        }
        urls_info = SerpUrlsInfo.deserialize(ser_urls)
        assert urls_info.qid == 100500
        assert len(urls_info.res_urls) == 1

    def test_deserialize_empty(self):
        ser_urls = {
            "qid": 100500
        }
        urls_info = SerpUrlsInfo.deserialize(ser_urls)
        assert urls_info.qid == 100500
        assert len(urls_info.res_urls) == 0


# noinspection PyClassHasNoInit
class TestMarkupInfo:
    def test_serialize_regular(self):
        pmi = ResMarkupInfo(pos=1, scales={})
        ser_pmi = pmi.serialize()
        assert "pos" in ser_pmi
        assert "scales" not in ser_pmi
        assert "relQueryText" not in ser_pmi

    def test_serialize_related(self):
        scales = {"imageadd": ["http:/images.yandex.ru"]}
        pmi = ResMarkupInfo(pos=None, related_query_text="best search engine", scales=scales)
        ser_pmi = pmi.serialize()
        assert "pos" not in ser_pmi
        assert "url" not in ser_pmi
        assert "relQueryText" in ser_pmi
        assert "scales" in ser_pmi

    def test_deserialize_regular(self):
        ser_pmi = {
            "url": "http://yandex.ru",
            "pos": 1,
            "scales": {
                "RELEVANCE": "VITAL"
            }
        }
        res_markup = ResMarkupInfo.deserialize(ser_pmi)
        assert res_markup.pos == 1
        assert len(res_markup.scales) == 1
        assert res_markup.related_query_text is None

    def test_deserialize_related(self):
        ser_pmi = {
            "relQueryText": "best search engine"
        }
        pmi = ResMarkupInfo.deserialize(ser_pmi)
        assert pmi.pos is None
        assert len(pmi.scales) == 0
        assert pmi.related_query_text == "best search engine"


# noinspection PyClassHasNoInit
class TestUrlInfo:
    def test_serialize_regular(self):
        url_info = ResUrlInfo(pos=1, url="http://yandex.ru")
        ser_pmi = url_info.serialize()
        assert "pos" in ser_pmi
        assert "url" in ser_pmi
        assert "scales" not in ser_pmi
        assert "relQueryText" not in ser_pmi

    def test_deserialize_regular(self):
        ser_pmi = {
            "url": "http://yandex.ru",
            "pos": 1
        }
        res_markup = ResUrlInfo.deserialize(ser_pmi)
        assert res_markup.pos == 1
        assert res_markup.url == "http://yandex.ru"

    def test_deserialize_related(self):
        ser_url = {
            "relQueryText": "best search engine"
        }
        pmi = ResUrlInfo.deserialize(ser_url)
        assert pmi.pos is None
        assert pmi.url is None
