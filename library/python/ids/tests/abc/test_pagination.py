# coding: utf-8
from __future__ import unicode_literals

import itertools

from pretend import stub
import pytest

from ids.lib.pagination import ResultSet
from ids.lib.linked_pagination import LinkedPage
from ids.services.abc.pagination import LinkedPageAbcFetcher


@pytest.fixture
def connector_services():
    def fake_get(params, *args, **kwargs):
        page = params.get('page', 1)
        url = 'https://abc-back.yandex-team.ru/api/v3/services/'
        return {
            'previous': url if page == 2 else None,
            'next': url + '?page=2' if page == 1 else None,
            'results': [
                {'on_page': page},
                {'on_page': page},
            ],
        }
    return stub(get=fake_get)


@pytest.fixture
def connector_members():
    def fake_get(params, *args, **kwargs):
        cursor = params.get('cursor')
        url = 'https://abc-back.yandex-team.ru/api/v4/services/members/'
        page = int(cursor.split('-')[0]) if cursor else 1
        return {
            'previous': url if page == 2 else None,
            'next': url + '?cursor=2-0' if page == 1 else None,
            'results': [
                {'on_page': page},
                {'on_page': page},
            ],
        }
    return stub(get=fake_get)


def fetcher(connector):
    return LinkedPageAbcFetcher(
        connector=connector,
        resource='DUMMY_RESOURCE',
        params={}
    )


@pytest.mark.parametrize('connector', [connector_services, connector_members])
def test_result_set_fetch_no_page_in_lookup(connector):
    result_set = ResultSet(fetcher(connector()))

    page = result_set.first_page
    assert page.num == 1


@pytest.mark.parametrize('connector', [connector_services, connector_members])
def test_result_set_fetch_all_pages(connector):
    result_set = ResultSet(fetcher(connector()))

    pages = list(result_set.get_pages())
    assert len(pages) == 2


@pytest.mark.parametrize('connector', [connector_services, connector_members])
def test_result_set_fetch_all_pages_check_content(connector):
    result_set = ResultSet(fetcher(connector()))

    pages = list(result_set.get_pages())
    expected_results = [{'on_page': x} for x in [1, 1, 2, 2]]
    results = list(itertools.chain.from_iterable(list(p) for p in pages))
    assert results == expected_results
