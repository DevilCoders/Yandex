import pytest
import requests
from urlparse import urljoin
from hamcrest import assert_that, equal_to

from antiadblock.cryprox.cryprox.config.system import SEED_HEADER_NAME, PARTNER_TOKEN_HEADER_NAME, FETCH_URL_HEADER_NAME


parsed_expected_query = "redir-setuniq=1&adb_enabled=1&pcode-test-ids-from-count=617270%2C0%2C13&format-type=13&tag=test_extuid_tag&test-tag=562950031199089&adb-bits=1"


@pytest.mark.parametrize('method', ('get', 'post'))
@pytest.mark.parametrize('url,expected_query', (
    ('http://mobile.yandexadexchange.net:80/v4/ad?uuid=1', 'uuid=1'),
    ('http://mobile.yandexadexchange.net:80/v1/startup?model=MI%209', 'model=MI%209'),
    ('http://adsdk.yandex.ru:80/v4/ad', ''),
    ('http://an.yandex.ru/count/WP0ejI?test-tag=77777777&pcode-test-ids-from-count=617270,0,13&format-type=13', parsed_expected_query),
    ('http://an.yandex.ru/count/WP0ejI?test-tag=77777777%26pcode-test-ids-from-count%3D617270%2C0%2C13%26format-type%3D13', parsed_expected_query),
    ('http://an.yandex.ru/rtbcount/WP0ejI?test-tag=77777777%26pcode-test-ids-from-count%3D617270%2C0%2C13%26format-type%3D13', parsed_expected_query),
))
def test_appcry_request(stub_server, cryprox_worker_url, get_config, method, url, expected_query):
    request_method = getattr(requests, method)

    def handler(**request):
        headers = {'x-aab-query': request.get('query', '')}
        return {'text': '', 'code': 200, 'headers': headers}

    stub_server.set_handler(handler)
    test_config = get_config('test_local')
    seed = 'my2007'
    headers = {
        'host': 'test.local',
        PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
        SEED_HEADER_NAME: seed,
        FETCH_URL_HEADER_NAME: url,
    }
    proxied = request_method(urljoin(cryprox_worker_url, '/appcry'), headers=headers)
    assert proxied.status_code == 200
    assert_that(proxied.headers['x-aab-query'], equal_to(expected_query))
