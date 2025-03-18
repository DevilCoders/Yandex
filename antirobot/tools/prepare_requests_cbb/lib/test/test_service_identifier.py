import pytest

from antirobot.tools.prepare_requests_cbb.lib import service_identifier


@pytest.mark.parametrize("url, service", (
    ("beta.yandex.ru/video", "video"),
    ("yandsearch.yandex.ru/video", "video"),
    ("yandex.ru/video", "video"),
    ("www.yandex.com.tr/video", "video"),
    ("video.yandex.ru", "video"),
    ("m.video.yandex.ru", "video"),

    ("beta.yandex.ru", "web"),
    ("yandsearch.yandex.ru", "web"),
    ("copy.yandex.net", "web"),
    ("people.yandex.ru", "web"),
    ("phone-search.yandex.ru", "web"),
    ("rca.yandex.ru", "web"),
    ("something.rca.yandex.ru", "web"),
    ("rrsearch.yandex.ru", "web"),
    ("yandex.ru/search", "web"),
    ("www.yandex.ru/search", "web"),
    ("yandex.by/search", "web"),
    ("yandex.kz/search", "web"),
    ("yandex.ru/search", "web"),
    ("yandex.st/search", "web"),
    ("yandex.ua/search", "web"),
    ("beta.yandex.ru/yandsearch", "web"),
    ("yandex.ru/search?ui=webmobileapp.yandex", "web"),
    ("yandex.ru/searchui=webmobileapp.yandex", "web"),
    ("yandex.ru/searchui=webmobileapp.yandex&some_text", "web"),
    ("yandex.ru/searchsome_text&ui=webmobileapp.yandex", "web"),

    ("www.yandex.ru", "morda"),
    ("yandex.by", "morda"),
    ("yandex.kz", "morda"),
    ("yandex.ru", "morda"),
    ("yandex.st", "morda"),
    ("yandex.ua", "morda"),

    ("ya.ru", "yaru"),

    ("video-xmlsearch.yandex.ru", "xml_c"),
    ("xmlsearch.yandex.ru", "xml_c"),
    ("seznam.xmlsearch.yandex.com", "xml_c"),
    ("xmlsearch.yandex.com.tr", "xml_c"),
    ("xmlsearch.hamster.yandex.com.tr", "xml_c"),

    ("clck.yandex.com", "click"),
    ("clck.yandex.ru", "click"),

    ("images.yandex.by", "img"),
    ("images.yandex.kz", "img"),
    ("images.yandex.ru", "img"),
    ("images.yandex.ua", "img"),
    ("images.yandex.com", "img"),
    ("images.yandex.com.tr", "img"),
    ("m.images.yandex.by", "img"),
    ("m.images.yandex.ru", "img"),
    ("m.images.yandex.ua", "img"),
    ("gorsel.yandex.com.tr", "img"),
    ("m.gorsel.yandex.com.tr", "img"),
    ("m.images.yandex.com", "img"),
    ("beta.yandex.ru/images", "img"),
    ("yandsearch.yandex.ru/images", "img"),
    ("yandex.ru/images", "img"),
    ("yandex.com.tr/images", "img"),
    ("yandex.com.tr/gorsel", "img"),
    ("www.yandex.com.tr/images", "img"),
    ("www.images.yandex.com.tr", "img"),
    ("www.yandex.com.tr/images", "img"),

    ("m.news.yandex.ru", "news"),
    ("news.yandex.by", "news"),
    ("news.yandex.kz", "news"),
    ("news.yandex.ru", "news"),
    ("news.yandex.ua", "news"),
    ("pda.news.yandex.by", "news"),
    ("pda.news.yandex.ru", "news"),
    ("pda.news.yandex.ua", "news"),
    ("wap.news.yandex.ru", "news"),
    ("wap.news.yandex.by", "news"),

    ("search.yaca.yandex.ru", "yaca"),

    ("blogs.yandex.by", "blogs"),
    ("blogs.yandex.ru", "blogs"),
    ("blogs.yandex.ua", "blogs"),
    ("m.blogs.yandex.ru", "blogs"),
    ("m.blogs.yandex.ua", "blogs"),
    ("pda.blogs.yandex.ru", "blogs"),

    ("hghltd.yandex.net", "hiliter"),

    ("market.yandex.ru", "market"),
    ("market.yandex.com.tr", "market"),
    ("m.market.yandex.com", "market"),

    ("rca.yandex.com", "rca"),

    ("yandex.ru/bus", "bus"),

    ("yandex.com.am/weather", "pogoda"),
    ("yandex.com.ge/weather", "pogoda"),
    ("yandex.co.il/weather", "pogoda"),
    ("yandex.com/weather", "pogoda"),
    ("yandex.ru/pogoda", "pogoda"),
    ("yandex.com.tr/hava", "pogoda"),

    ("yandex.ru/video/search?ui=webmobileapp.yandex", "searchapp"),
    ("yandex.ru/searchapp?ui=111&some_text", "web"),

    ("yandex.ru/collections/api/v0.1/link-statussource_name=turbo&ui=touch", "collections"),
    ("collections.yandex.ru/collections/api/v0.1/recent-timestampsource_name=yabrowser&source_version=20.3.0.1223&ui=desktop", "collections"),

    ("yandex.ru/tutor", "tutor"),
))
def test_get_service(url, service):
    assert service_identifier.ServiceIdentifier().get_service(url) == service
