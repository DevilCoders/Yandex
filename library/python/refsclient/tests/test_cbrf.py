# -*- encoding: utf-8 -*-
from __future__ import unicode_literals

from refsclient.exceptions import ApiCallError
from refsclient.http import RefsResponse


def test_shortcuts(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    cbrf = refs.get_ref_cbrf()

    assert cbrf.get_types()
    assert cbrf.get_resources()
    assert cbrf.get_type_fields('Bank')


def test_get_banks(refs):
    refs = refs()
    cbrf = refs.get_ref_cbrf()

    results = cbrf.get_banks_info(['045004641', '044525225'])
    assert len(results) == 2
    assert 'nameFull' in results['045004641']
    assert 'bic' in results['044525225']


def test_listing(refs):
    refs = refs()

    class context:
        page = 0
        items_per_page = 3
        pages_count = 5

    def listing_mock(*args, **kwargs):
        context.page = int((kwargs.get('params', {'page': '1'}).get('page', '1')))

        if 0 > context.page > context.pages_count:
            raise ApiCallError(message='Page not found', status_code=404)

        class response:
            links = {
                'last': {'rel': 'last', 'url': 'http://testserver?page=%s' % context.pages_count},
                'first': {'rel': 'first', 'url': 'http://testserver?page=1'},
                'next': {'rel': 'first', 'url': 'http://testserver?page=%s' % (context.page + 1)},
            }

        if context.page == context.pages_count:
            response.links.pop('next')
            response.links.pop('last')

        return RefsResponse({
            'data': {
                'listing':[
                    {'restricted': False, 'nameFull': 'Bank #%s.1' % i, 'bic': '%s1' % i}
                    for i in range(context.items_per_page)
                ]
            }
        }, response=response)

    cbrf = refs.get_ref_cbrf()
    cbrf._request = listing_mock
    results = list(cbrf.banks_listing(['restricted', 'nameFull', 'bic']))

    assert context.page == context.pages_count
    assert len(results) == context.pages_count*context.items_per_page
