#!/usr/bin/env python
# coding: utf-8
from __future__ import division, absolute_import, print_function, unicode_literals
import uuid
import requests
from statface_client.tools import pretty_dump, dump_raw_request
import six
from six.moves import zip


def test_pretty_dump():
    obj = dict(a=six.text_type(uuid.uuid4()), b=(1, 2), c=u'лалала')
    pretty_dump(obj)


def test_raw_request_dump():
    req = requests.Request(
        url='http://imaginary_host.ru',
        headers={1: 2},
        params=dict(
            penguin=1,
            gentoo=dict(
                field='avoid_spaces',
                field_two='avoid_spaces_more'
            )
        ),
        data=[1, 2, 3]
    )

    expected = [
        '  request get url: http://imaginary_host.ru',
        '  headers: {',
        '    "1": 2',
        '  }',
        '  params: {',
        '    "gentoo": {',
        '      "field": "avoid_spaces",',
        '      "field_two": "avoid_spaces_more"',
        '    },',
        '    "penguin": 1',
        '  }',
        '  data: [',
        '    1,',
        '    2,',
        '    3',
        '  ]'
    ]

    req_dump = dump_raw_request(req)

    assert isinstance(req_dump, six.text_type)

    print(req_dump)
    for got_line, expected_line in zip(req_dump.split('\n'), expected):
        assert got_line == expected_line
