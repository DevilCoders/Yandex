import pytest
import yatest.common

from library.infected_masks import masks_comptrie


@pytest.fixture(params=[
    (b'http://0gdz.ru/', True),
    (b'http://yandex.ru/', False),
    (b'http://google.com', False),
    (b'https://www.1mbetcool.win/su/siteaccess.htm', True),
])
def test_case(request):
    return request.param


def test_from_file(test_case):
    trie = masks_comptrie.CategMaskComptrie.from_file(
        yatest.common.runtime.work_path(u"fixtures/test_trie/suggest_rkn.trie"))
    url, in_trie = test_case
    assert (url in trie) == in_trie


def test_from_bytes(test_case):
    with open(yatest.common.runtime.work_path(u"fixtures/test_trie/suggest_rkn.trie"), u'rb') as f:
        trie = masks_comptrie.CategMaskComptrie.from_bytes(f.read())
    url, in_trie = test_case
    assert (url in trie) == in_trie
