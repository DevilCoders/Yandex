# coding: utf-8
from __future__ import unicode_literals

import itertools

from pretend import stub
import pytest

from ids.lib.pagination import ResultSet
from ids.lib.linked_pagination import LinkedPageFetcher, LinkedPage


@pytest.fixture
def connector():
    def fake_get(params, *args, **kwargs):
        page = params.get('page', 1)
        return {
            'count': 3,
            'previous': 'https://goals/' if page == 2 else None,
            'next': 'https://goals/?page=2' if page == 1 else None,
            'results': [
                {'on_page': page},
                {'on_page': page},
            ],
        }
    return stub(get=fake_get)


@pytest.fixture
def fetcher(connector):
    return LinkedPageFetcher(
        connector=connector,
        resource='DUMMY_RESOURCE',
        params={}
    )


def test_page_cls_methods():
    data = {
        'count': 3,
        'previous': None,
        'next': '_next_page_url_',
        'results': ['a', 'b', 'c']
    }

    page = LinkedPage(data=data)

    assert page.count == 3
    assert page.previous_page_url is None
    assert page.next_page_url == '_next_page_url_'
    assert page.results == ['a', 'b', 'c']

    assert page.num == 1
    assert page.total == 3
    assert len(page) == 3


def test_result_set_fetch_no_page_in_lookup(fetcher):
    result_set = ResultSet(fetcher)

    page = result_set.first_page
    assert page.num == 1


def test_result_set_fetch_all_pages(fetcher):
    result_set = ResultSet(fetcher)

    pages = list(result_set.get_pages())
    assert len(pages) == 2


def test_result_set_fetch_all_pages_check_content(fetcher):
    result_set = ResultSet(fetcher)

    pages = list(result_set.get_pages())
    expected_results = [{'on_page': x} for x in [1, 1, 2, 2]]
    results = list(itertools.chain.from_iterable(list(p) for p in pages))
    assert results == expected_results
