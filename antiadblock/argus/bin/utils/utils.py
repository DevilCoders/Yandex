import re
from time import gmtime, strftime

from antiadblock.argus.bin.utils.exceptions import ParametersCookieException
from antiadblock.libs.adb_selenium_lib.schemas import Cookie
from antiadblock.libs.utils.utils import parse_and_validate_cookies


def parse_cookies(cookies: str) -> list[Cookie]:
    try:
        parsed = parse_and_validate_cookies(cookies)
    except Exception as e:
        raise ParametersCookieException(str(e))
    else:
        return [
            Cookie(
                name=cookie['name'],
                value=cookie['value'],
                path=cookie.get('path'),
                secure=cookie.get('secure', False),
            )
            for cookie in parsed
        ]


def get_404_page(url) -> str:
    domain = re.match(r'http(?:s|)://(?:www\.|)[\w\.\-]+', url).group()

    if "yandex" in domain:
        return "https://yandex.ru/404page"

    if "liveinternet" in domain:
        return "https://www.liveinternet.ru/rating/ru/"

    # todo sdamgia, drive2, echomsk, smi24 https://st.yandex-team.ru/ANTIADB-2776

    return domain + "/404page"


def get_cookie_domain(url: str) -> str:
    """
    >>> get_cookie_domain('http://lena-miro.ru')
    'lena-miro.ru'
    >>> get_cookie_domain('http://sub1.sub2.yandex.ru')
    'sub1.sub2.yandex.ru'
    >>> get_cookie_domain('http://www.yandex.ru/asdfasd?arg=1&arg2=2')
    'www.yandex.ru'
    """
    host = re.match(r'^(([^:/?#]+)?:)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?', url).group(4)
    return host


def current_time():
    return strftime('%Y-%m-%dT%H:%M:%S', gmtime())
