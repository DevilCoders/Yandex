from time import sleep

import pytest
import requests
from urlparse import urljoin

from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config import service as service_config
from antiadblock.cryprox.cryprox.service import action as ca
from antiadblock.cryprox.cryprox.service.metrics import SYSTEM_METRICS_TIME_WINDOW_SIZE
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST


def count_actions(metrics_url, actions, labels=None):
    labels = labels or dict()
    sleep(5 * service_config.METRICS_TRANSFER_DELAY)  # wait for metrics to aggregate in service app
    count = dict()
    sensors = requests.get(metrics_url).json()['sensors']
    for action in actions:
        count[action] = sum([
            sum(s['value'] for s in sensor['timeseries']) for sensor in sensors
            if action == sensor['labels']['action'] and sensor['labels']['sensor'] == 'action_counter'
               and all([sensor['labels'][label_name] == label_value for label_name, label_value in labels.iteritems()])
        ])
    return count


@pytest.mark.parametrize(
    'link, expected_actions, not_expected_actions', [
        # only fetch content, without 'bodycrypt' and 'bodyreplace'
        ('http://{}/test.bin'.format(TEST_LOCAL_HOST),
         [ca.DECRYPTURL, ca.CHKACCESS, ca.HANDLER_CRY, ca.FETCH_CONTENT],
         [ca.ACCELREDIRECT, ca.BODYCRYPT, ca.BODYREPLACE, ca.HANDLER_NONE]),
        # length crypting
        ('http://{}/test.bin'.format(TEST_LOCAL_HOST),
         [ca.DECRYPTURL, ca.CHKACCESS, ca.HANDLER_CRY, ca.FETCH_CONTENT],
         [ca.ACCELREDIRECT, ca.BODYCRYPT, ca.BODYREPLACE, ca.HANDLER_NONE]),
        # fetch content with 'bodycrypt'
        ('http://{}/index.html'.format(TEST_LOCAL_HOST),
         [ca.DECRYPTURL, ca.CHKACCESS, ca.HANDLER_CRY, ca.FETCH_CONTENT, ca.BODYCRYPT, ca.BODYREPLACE],
         [ca.ACCELREDIRECT, ca.HANDLER_NONE]),
        # fetch image that matches ACCEL_REDIRECT_URL_RE
        ('http://{}/accel-images/test.png'.format(TEST_LOCAL_HOST),
         [ca.DECRYPTURL, ca.CHKACCESS, ca.ACCELREDIRECT],
         [ca.HANDLER_CRY, ca.FETCH_CONTENT, ca.HANDLER_NONE]),
        # PCODE replace
        ('http://yastatic.net/partner-code/PCODE_CONFIG.js',
         [ca.HANDLER_CRY, ca.FETCH_CONTENT, ca.BODYCRYPT, ca.BODYREPLACE, ca.JS_REPLACE],
         [ca.HANDLER_NONE]),
        ('http://yastat.net/partner-code/PCODE_CONFIG.js',
         [ca.HANDLER_CRY, ca.FETCH_CONTENT, ca.BODYCRYPT, ca.BODYREPLACE, ca.JS_REPLACE],
         [ca.HANDLER_NONE]),
    ])
def test_solomon_metrics(stub_server, expected_actions, not_expected_actions, link, get_key_and_binurlprefix_from_config, get_config, cryprox_service_url):
    """
    Test correct metric increasing after making {number_of_requests} requests.
    """
    metrics_url = urljoin(cryprox_service_url, '/metrics')
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, link, key, enable_trailing_slash=True)
    count_before = count_actions(metrics_url, expected_actions + not_expected_actions)
    number_of_requests = 20
    for i in range(number_of_requests):
        requests.get(crypted_link, headers={"host": TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    count_after = count_actions(metrics_url, expected_actions + not_expected_actions)

    for action in expected_actions:
        assert count_after[action] == count_before[action] + number_of_requests
    for action in not_expected_actions:
        assert count_after[action] == count_before[action]


@pytest.mark.parametrize(
    'link', [
        ('http://{}/test.bin'.format(TEST_LOCAL_HOST)),
    ])
def test_system_metrics(stub_server, link, get_key_and_binurlprefix_from_config, get_config, cryprox_service_url):
    def get_metrics():
        metrics = requests.get(urljoin(cryprox_service_url, '/system_metrics')).json()
        return metrics

    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, link, key, enable_trailing_slash=True)
    sleep(SYSTEM_METRICS_TIME_WINDOW_SIZE + 1)  # wait for metrics to be cleared
    number_of_requests = 4
    for i in range(number_of_requests):
        requests.get(crypted_link, headers={"host": TEST_LOCAL_HOST,
                                            system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    sleep(2)  # wait for metrics to be aggregated
    metrics = get_metrics()
    rps_diff = metrics['rps']
    assert number_of_requests / 2 <= rps_diff <= number_of_requests
    assert metrics['cpu_load'] > 0


@pytest.mark.parametrize('url, cookies, expected_present, expected_absent', [
    ('https://an.yandex.ru/meta/zapros_za_reklamoy', {'yandexuid': 'est takoe'}, 1, 0),
    ('https://an.yandex.ru/meta/zapros_za_reklamoy', {'i': 'est takoe'}, 1, 0),
    ('https://an.yandex.ru/meta/zapros_za_reklamoy', {'crookie': 'est takoe'}, 1, 0),
    ('https://an.yandex.ru/meta/zapros_za_reklamoy', {'yandexuid': 'est takoe', 'i': 'tozhe est'}, 1, 0),
    ('https://an.yandex.ru/meta/zapros_za_reklamoy', {'drugaya_kuka': 'prisutstvuet'}, 0, 1),
    ('https://test.local/partenrskiy_zapros', {'yandexuid': 'prisutstvuet'}, 0, 0),
])
def test_check_uid_cookies_metrics(stub_server, get_config, get_key_and_binurlprefix_from_config, cryprox_service_url, url, cookies, expected_present, expected_absent):
    metrics_url = urljoin(cryprox_service_url, '/metrics')
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    count_present_before = count_actions(metrics_url, [ca.CHECK_UID_COOKIES], {'uid_cookie_present': 1})[ca.CHECK_UID_COOKIES]
    count_absent_before = count_actions(metrics_url, [ca.CHECK_UID_COOKIES], {'uid_cookie_present': 0})[ca.CHECK_UID_COOKIES]

    crypted_link = crypt_url(binurlprefix, url, key, enable_trailing_slash=True)
    requests.get(crypted_link, cookies=cookies, headers={"host": TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    count_present_after = count_actions(metrics_url, [ca.CHECK_UID_COOKIES], {'uid_cookie_present': 1})[ca.CHECK_UID_COOKIES]
    count_absent_after = count_actions(metrics_url, [ca.CHECK_UID_COOKIES], {'uid_cookie_present': 0})[ca.CHECK_UID_COOKIES]

    assert count_present_after == count_present_before + expected_present
    assert count_absent_after == count_absent_before + expected_absent
