import pytest

from antiadblock.cryprox.cryprox.common.tools.ua_detector import parse_user_agent_data
from antiadblock.cryprox.cryprox.config.system import UserDevice


@pytest.mark.parametrize('user_agent, is_crawler', [
    (None, False),
    ('', False),
    ('Mozilla/5.0 (compatible; YandexDirect/3.0; +http://yandex.com/bots)', True),
    ('Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html)', True),
    ('Mozilla/5.0 (compatible; Linux x86_64; Mail.RU_Bot/2.0; +http://go.mail.ru/help/robots)', True),
    ('Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/73.0.3683.86 Safari/537.36', False),
    ('Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/72.0.3626.109 YaBrowser/19.3.0.2489 Yowser/2.5 Safari/537.36', False),
    ('Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322)', False),
    ('Mozilla/5.0 (compatible; YandexDirect/3.0; +http://yandex.com/bots)', True),
    ('Mozilla/5.0 (Linux; Android 6.0.1; Nexus 5X Build/MMB29P) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.96 Mobile Safari/537.36'
     ' (compatible; Googlebot/2.1; +http://www.google.com/bot.html)', True),
])
def test_detect_crawler(user_agent, is_crawler):
    assert parse_user_agent_data(user_agent).isRobot == is_crawler


@pytest.mark.parametrize('user_agent, is_mobile', [
    (None, False),
    ('', False),
    ('Mozilla/5.0 (compatible; YandexDirect/3.0; +http://yandex.com/bots)', False),
    ('Mozilla/5.0 (Linux; U; Android 2.2; ru-ru; Desire_A8181 Build/FRF91) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1', True),
    ('Mozilla/5.0 (Macintosh; Intel Mac OS X 10.14; rv:69.0) Gecko/20100101 Firefox/69.0', False),
    ('Mozilla/5.0 (iPhone; CPU iPhone OS 13_1_2 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.0.1 Mobile/15E148 Safari/604.1', True)
])
def test_detect_device(user_agent, is_mobile):
    ua_data = parse_user_agent_data(user_agent)
    assert ua_data.isMobile == is_mobile
    if is_mobile:
        assert ua_data.device == UserDevice.MOBILE
    else:
        assert ua_data.device == UserDevice.DESKTOP
