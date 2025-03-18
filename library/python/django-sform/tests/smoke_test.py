# encoding: utf-8
from __future__ import unicode_literals

from collections import OrderedDict

from .fixtures import form_data


def test_clean(form_cls):
    form = form_cls(data=form_data)
    assert form.is_valid()
    assert form.cleaned_data == form_data


def test_unclean(form_cls):
    form_data = {
        'int_field': 'a',
        'field_set': {
            'iner_int': 'a',
        },
        'field_set_grid': [
            {'iner_int': 'a'},
            {'iner_int': 'a'},
        ],
        'int_grid': ['a', 'a', 'a'],
    }
    form = form_cls(data=form_data)
    assert not form.is_valid()
    assert form.errors == {'errors': {
        'int_field': [{'code': 'invalid'}],
        'field_set[iner_int]': [{'code': 'invalid'}],
        'field_set_grid[0][iner_int]': [{'code': 'invalid'}],
        'field_set_grid[1][iner_int]': [{'code': 'invalid'}],
        'int_grid[0]': [{'code': 'invalid'}],
        'int_grid[1]': [{'code': 'invalid'}],
        'int_grid[2]': [{'code': 'invalid'}],
    }}


def test_data_as_dict(form_cls):
    assert form_cls(initial=form_data).data_as_dict() == OrderedDict([
        (
            'int_field', {
                'name': 'int_field',
                'value': 1
            }
        ), (
            'field_set', {
                'name': 'field_set',
                'value': OrderedDict([
                    (
                        'iner_int', {
                            'name': 'field_set[iner_int]',
                            'value': 2
                        }
                    )
                ])
            }
        ), (
            'field_set_grid', {
                'name': 'field_set_grid',
                'value': [
                    {
                        'name': 'field_set_grid[0]',
                        'value': OrderedDict([
                            (
                                'iner_int', {
                                    'name': 'field_set_grid[0][iner_int]',
                                    'value': 3
                                }
                            )
                        ])
                    }, {
                        'name': 'field_set_grid[1]',
                        'value': OrderedDict([
                            (
                                'iner_int', {
                                    'name': 'field_set_grid[1][iner_int]',
                                    'value': 4
                                }
                            )
                        ])
                    }
                ]
            }
        ), (
            'int_grid', {
                'name': 'int_grid',
                'value': [
                    {'name': 'int_grid[0]', 'value': 5},
                    {'name': 'int_grid[1]', 'value': 6},
                    {'name': 'int_grid[2]', 'value': 7}
                ]
            }
        )
    ])


def test_structure_as_dict(form_cls):
    assert form_cls().structure_as_dict() == OrderedDict([
        (
            'int_field', {
                'key': 'int_field',
                'type': 'integer',
                'value': '',
                'name': 'int_field'
            }
        ), (
            'field_set', {
                'key': 'field_set',
                'type': 'fieldset',
                'value': OrderedDict([
                    (
                        'iner_int', {
                            'key': 'iner_int',
                            'type': 'integer',
                            'value': '',
                            'name': 'field_set[iner_int]'
                        }
                    )
                ]),
                'name': 'field_set'
            }
        ), (
            'field_set_grid', {
                'key': 'field_set_grid',
                'type': 'grid',
                'value': [],
                'name': 'field_set_grid',
                'structure': {
                    'key': '0',
                    'type': 'fieldset',
                    'value': OrderedDict([
                        (
                            'iner_int', {
                                'key': 'iner_int',
                                'type': 'integer',
                                'value': '',
                                'name': 'field_set_grid[0][iner_int]'
                            }
                        )
                    ]),
                    'name': 'field_set_grid[0]'
                }
            }
        ), (
            'int_grid', {
                'key': 'int_grid',
                'type': 'grid',
                'value': [],
                'name': 'int_grid',
                'structure': {
                    'key': '0',
                    'type': 'integer',
                    'value': '',
                    'name': 'int_grid[0]'
                }
            }
        )
    ])
