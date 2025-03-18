# coding: utf-8
from __future__ import unicode_literals

import itertools

from pretend import stub
import pytest

from ids.lib.pagination import ResultSet
from ids.lib.paging_api import PagingApiPage, PagingApiFetcher


@pytest.fixture
def connector():
    def fake_get(params, *args, **kwargs):
        page = params.get('_page', 1)
        return {
            'page': page,
            'total': 6,
            'pages': 3,
            'result': [
                {'on_page': page},
                {'on_page': page},
            ],
            'limit': 2,
        }
    return stub(get=fake_get)


@pytest.fixture
def fetcher(connector):
    return PagingApiFetcher(
        connector=connector,
        resource='DUMMY_RESOURCE',
        params={}
    )


def test_page_cls_methods():
    data = {
        'page': 1,
        'pages': 3,
        'limit': 10,
        'total': 5,
        'result': ['a', 'b', 'c']
    }

    page = PagingApiPage(data=data)

    assert page.page == 1
    assert page.pages == 3
    assert page.limit == 10
    assert page.total == 5
    assert page.result == ['a', 'b', 'c']

    assert page.is_first
    assert not page.is_last
    assert len(page) == 3


def test_result_set_fetch_no_page_in_lookup(fetcher):
    result_set = ResultSet(fetcher)

    page = result_set.first_page
    assert page.num == 1


def test_result_set_fetch_all_pages(fetcher):
    result_set = ResultSet(fetcher)

    pages = list(result_set.get_pages())
    assert len(pages) == 3


def test_result_set_fetch_all_pages_check_content(fetcher):
    result_set = ResultSet(fetcher)

    pages = list(result_set.get_pages())
    expected_results = [{'on_page': x} for x in [1, 1, 2, 2, 3, 3]]
    results = list(itertools.chain.from_iterable(list(p) for p in pages))
    assert results == expected_results
