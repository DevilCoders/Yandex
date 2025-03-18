import mock
import json

from antiadblock.cryprox.cryprox.common.config_utils import read_configs_from_file_cache, CONFIG_CACHE

SAMPLE_CONFIGS = {"test_local": {u'statuses': [u'active', u'test'],
                                 u'version': 'test',
                                 u'config': {u'EXTUID_COOKIE_NAMES': [u'extuid', u'second-ext-uid'], u'id': u'test', u'ACCEL_REDIRECT_URL_RE': [u'/accel-images/\\w+\\.png$'],
                                             u'ENCRYPTION_STEPS': [0, 1, 2, 3], u'CRYPT_URL_RE': [u'(?:[\\w-]+\\.)?smi2\\.(?:ru|net)/.*?'],
                                             u'CLIENT_REDIRECT_URL_RE': [], u'ADFOX_DEBUG': True, u'EXTUID_TAG': u'test_extuid_tag', u'CRYPT_URL_PREFFIX': u'/',
                                             u'CRYPT_RELATIVE_URL_RE': [u'/static/[\\w/.-]+\\.(?:js|css|gif|png)(?:\\?(?:v=)?\\d*[_.-]?\\d*)?/?', u'/scripts/.*?', u'(?:\\.{0,2}/)*testing/.*?'],
                                             u'DETECT_ELEMS': {u'id': [u'mlph-banner', u'popup_box']}, u'CRYPT_SECRET_KEY': u'duoYujaikieng9airah4Aexai4yek4qu', u'CM_TYPE': 0,
                                             u'DETECT_LINKS': [{u'src': u'http://test.local/some.js', u'type': u'get'}], u'PROXY_URL_RE': [u'test\\.local/.*', u'this-is-a-fake-address\\.com/.*'],
                                             u'FOLLOW_REDIRECT_URL_RE': [u'(?:https?://)?(?:[\\w-]+\\.)?smi2\\.(?:ru|net)/.*?'], u'EXCLUDE_COOKIE_FORWARD': [u'test_cookie'],
                                             u'CRYPT_BODY_RE': {u'\\bsmi2adblock[-\\w]+?\\b': None}, u'CRYPT_ENABLE_TRAILING_SLASH': True, u'PARTNER_TOKENS': [u'test_token1', u'test_token2'],
                                             u'RANDOM_SLASH_INSERT': False, u'PUBLISHER_SECRET_KEY': u'eyJhbGciOi'}}}


def test_cached_read():
    with mock.patch('__builtin__.open', mock.mock_open(read_data=json.dumps(SAMPLE_CONFIGS)), create=True) as m:
        result = read_configs_from_file_cache()
        m.assert_called_once_with(CONFIG_CACHE, "r")
        assert result == SAMPLE_CONFIGS
