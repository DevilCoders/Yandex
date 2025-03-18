# -*- encoding: utf-8 -*-
from __future__ import unicode_literals
from datetime import datetime


def test_shortcuts(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    currency = refs.get_ref_currency()

    assert currency.get_types()
    assert currency.get_resources()
    assert currency.get_type_fields('Rate')


def test_get_rates(refs):
    refs = refs()
    currency = refs.get_ref_currency()

    results = currency.get_rates_info(dates=[datetime(2019, 1, 1)], codes=['USD'])

    assert results


def test_get_listing(refs):
    refs = refs()
    currency = refs.get_ref_currency()

    results = currency.get_listing()
    assert results
    assert 'RUB' in results


def test_get_sources(refs):
    refs = refs()
    currency = refs.get_ref_currency()

    results = currency.get_sources()
    assert results
    assert 'RUS' in results
