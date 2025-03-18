import functools

import yaqutils.url_helpers as uurl
import yaqutils.math_helpers as umath

import mstand_utils.mstand_misc_helpers as mstand_umisc


# scipy.special.expit replacement
def expit(x):
    """
    :type x: float | int
    :rtype: float
    """
    return umath.expit(x)


# numpy.argmax for 1D arrays
def argmax(array):
    return umath.argmax(array)


# numpy.argmin for 1D arrays
def argmin(array):
    return umath.argmin(array)


# mean (average) value of array (NOT median!)
def mean(array):
    return umath.avg(array)


# median (0.5 percentile) of array (NOT mean)
def median(array):
    return umath.median(array)


# python2/3 compatible wrapper
def urlparse(url):
    """
    :type url: str
    :rtype: tuple
    """
    return uurl.urlparse(url)


# python2/3 compatible wrapper
def parse_qs(url):
    """
    :type url: str
    :rtype:
    """
    return uurl.parse_qs(url)


# python2/3 compatible wrapper
def reduce(function, iterable):
    return functools.reduce(function, iterable)


def get_yandex_service_by_url(url):
    """
    :type url: str
    :rtype: str
    """
    if not url:
        return ""

    yandex_urls = [
        "ya.ru",
        "yandex.by",
        "yandex.com",
        "yandex.com.tr",
        "yandex.ee",
        "yandex.kz",
        "yandex.md",
        "yandex.ru",
        "yandex.ua",
        "yandex.uz",
    ]

    yandex_suffixes = [
        ".ya.ru",
        ".yandex.by",
        ".yandex.com",
        ".yandex.com.tr",
        ".yandex.ee",
        ".yandex.kz",
        ".yandex.md",
        ".yandex.ru",
        ".yandex.ua",
        ".yandex.uz",
    ]

    yandex_services = [
        "adv",
        "afisha",
        "alice",
        "auto",
        "avia",
        "blog",
        "blogs",
        "browser",
        "collections",
        "disk",
        "education",
        "efir",
        "games",
        "health",
        "images",
        "local",
        "mail",
        "maps",
        "market",
        "metro",
        "msearch",
        "music",
        "news",
        "oplata",
        "padsearch",
        "plus",
        "pogoda",
        "q",
        "rabota",
        "radio",
        "rasp",
        "realty",
        "search",
        "slovari",
        "sport",
        "support",
        "talents",
        "taxi",
        "time",
        "touchsearch",
        "trains",
        "translate",
        "travel",
        "turbo",
        "tutor",
        "tv",
        "uslugi",
        "video",
        "weather",
        "yandsearch",
        "zen",
        "znatoki",
    ]

    yandex_sites = [
        "auto.ru",
        "beru.ru",
        "eda.yandex",
        "edadeal.ru",
        "kinopoisk.ru",
        "thequestion.ru",
    ]

    host = mstand_umisc.extract_host_from_url(url, normalize=True)
    is_yandex_url = host in yandex_urls
    ends_with_yandex_url = any([host.endswith(suffix) for suffix in yandex_suffixes])

    for service in yandex_services:
        # *.yandex.ru case
        if ends_with_yandex_url and (host.startswith(service) or host.startswith("m." + service) or host.startswith("t." + service)):
            return service
        # yandex.ru/* case
        if is_yandex_url:
            path_query = url.split(host)[-1].lstrip('/')
            if path_query.startswith(service):
                return service

    if host in yandex_sites:
        if host == "auto.ru":
            return "auto"
        return host

    return ""
