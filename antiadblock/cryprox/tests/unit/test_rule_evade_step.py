# coding=utf-8
import pytest

from antiadblock.libs.decrypt_url.lib import resplit_using_length
from antiadblock.cryprox.cryprox.url.transform.rule_evade_step import encode, construct_forward_evade_regex

test_rules = ['-ad2_',
              '/_ads/',
              '_openx/',
              '/ad2_']


@pytest.mark.parametrize('url, encoded_etalon_url',
                         [
                             ("/ads/_openx//ad2_", "/ads/_$openx//$ad2_"),
                             ("/ads/_openx/ad2_", "/ads/_$openx/ad2_")  # правила внахлест не должны избегаться
                         ])
def test_evader_evades(url, encoded_etalon_url):
    encoded = encode(url, construct_forward_evade_regex(test_rules))
    assert encoded == encoded_etalon_url
    length = 13
    new_url, _ = resplit_using_length(length, encoded)
    assert '$' not in new_url
    assert new_url == url.replace('/', '')  # resplit_using_length убирает слеши
