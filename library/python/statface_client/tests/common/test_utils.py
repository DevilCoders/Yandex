# coding: utf8
"""
...
"""

from __future__ import division, absolute_import, print_function, unicode_literals

import pytest

from statface_client.utils import force_text
import six
from six.moves import range


FORCE_TEXT_TEST_DATA_REPL = (  # (test_input, expected)
    (None, 'None'),
    (
        b''.join(six.int2byte(val) for val in range(256)),
        (u'\ufffd\t\n\ufffd\r\ufffd !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ'
         u'[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~') + u'\ufffd' * 129,
    ),
    (u'тест', u'тест'),
    (u'abc\udc34xyz', u'abc\ufffdxyz'),
)


@pytest.mark.parametrize("idx", range(len(FORCE_TEXT_TEST_DATA_REPL)))
def test_force_text_repl(idx):
    test_input, expected = FORCE_TEXT_TEST_DATA_REPL[idx]
    assert force_text(test_input, errors='replace') == expected


# At least ensure they run successfully:
@pytest.mark.parametrize("idx", range(len(FORCE_TEXT_TEST_DATA_REPL)))
def test_force_text_ign(idx):
    test_input, _ = FORCE_TEXT_TEST_DATA_REPL[idx]
    force_text(test_input, errors='ignore')
