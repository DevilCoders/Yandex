# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from difflib import unified_diff

import six
from lxml import etree

XML_ASSERTION_ERROR = '''XML documents mismatch
Expected:
{expected}

Actual:
{actual}

Diff:
{diff}
'''


def assert_xml_equals(actual, expected):
    """
    Assert that XML documents equal.
    """
    actual_lines = _normalize_xml(actual).splitlines(True)
    expected_lines = _normalize_xml(expected).splitlines(True)
    diff = ''.join(unified_diff(expected_lines, actual_lines, 'expected', 'actual'))
    if diff:
        raise AssertionError(
            XML_ASSERTION_ERROR.format(
                actual=six.ensure_text(actual), expected=six.ensure_text(expected), diff=six.ensure_text(diff)
            )
        )


def _normalize_xml(str_value):
    str_value = six.ensure_text(str_value).strip()
    xml_value = etree.fromstring(str_value.encode('utf-8'), etree.XMLParser(remove_blank_text=True))
    return etree.tounicode(xml_value, pretty_print=True)
