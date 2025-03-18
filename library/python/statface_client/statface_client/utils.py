# coding: utf8
"""
...
"""

from __future__ import division, absolute_import, print_function, unicode_literals

# from builtins import *  # pylint: disable=redefined-builtin,unused-wildcard-import,wildcard-import


import re
import six


# https://bitbucket.org/openpyxl/openpyxl/src/7e42546674ebeb0e518d1a058abbb1a6d6f71c/openpyxl/cell/cell.py?at=default&fileviewer=file-view-default#cell.py-69
# ILLEGAL_CHARACTERS = r'([\000-\010]|[\013-\014]|[\016-\037])+'
# Simplified, and with \x7f ('DEL') too:
# http://www.fileformat.info/info/unicode/char/7f/index.htm
ILLEGAL_CHARACTERS = r'[\000-\010\013-\014\016-\037\177]+'

# https://stackoverflow.com/q/18673213
# Only occasionally problematic.
# Example:
#
#     import json; json.loads(json.dumps("abc\xed\xb0\xb4xyz".decode('utf-8')))
#
LONE_SURROGATES = (
    r'([\ud800-\udbff](?![\udc00-\udfff]))|'
    r'((?<![\ud800-\udbff])[\udc00-\udfff])')

# ILLEGAL_CHARACTERS_RE = re.compile(ILLEGAL_CHARACTERS)
# LONE_SURROGATES_RE = re.compile(LONE_SURROGATES)
ALL_ILLEGAL_CHARACTERS_RE = re.compile('{}|{}'.format(ILLEGAL_CHARACTERS, LONE_SURROGATES))


def _raise_strict(match):
    value = match.group(0)
    raise UnicodeDecodeError("Invalid characters: {!r}".format(value))


ERR_TO_REPLACEMENT = dict(
    replace=u"\ufffd",
    ignore=u"",
    strict=_raise_strict)


def force_text(value, errors='replace', encoding='utf-8', allow_non_string=True):
    """
    Given any value, make a valid text (unicode) string out of it.

    :param errors: "ignore"|"replace"|"strict"

    :param allow_non_string: whether non-string values should be processed
    rather than raised.
    """
    repl = ERR_TO_REPLACEMENT[errors]

    if isinstance(value, bytes):
        value = value.decode(encoding, errors=errors)

    if not isinstance(value, six.text_type):
        if not allow_non_string:
            raise ValueError("Value is not a string", value)
        try:
            value = six.text_type(value)
        except Exception:  # pylint: disable=broad-except
            value = repr(value).decode('utf-8', errors=errors)

    assert isinstance(value, six.text_type)

    value = ALL_ILLEGAL_CHARACTERS_RE.sub(repl, value)
    return value
