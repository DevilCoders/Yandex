# -*- encoding: utf-8 -*-
from __future__ import unicode_literals


def test_shortcuts(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    swift = refs.get_ref_swift()

    assert swift.get_types()
    assert swift.get_resources()
    assert swift.get_type_fields('Event')


def test_get_bics(refs):
    refs = refs()
    swift = refs.get_ref_swift()

    results = swift.get_bics_info(['YNDMRUM1', 'SECTAEA1710'])
    assert len(results) == 2
    assert 'instName' in results['SECTAEA1710']
    assert 'bicBranch' in results['YNDMRUM1XXX']


def test_get_holidays(refs):
    refs = refs()
    swift = refs.get_ref_swift()

    results = swift.get_holidays_info(
        date_since='2021-04-01',
        date_to='2021-04-25',
        countries=['RU'],
    )
    assert len(results) == 8

    for result in results:
        assert result['countryCode'] == 'RU'
