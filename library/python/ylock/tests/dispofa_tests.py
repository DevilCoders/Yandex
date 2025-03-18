from __future__ import absolute_import

import os

from library.python.ylock.tests import base

try:
    import dispofa
except ImportError:
    from unittest.case import SkipTest

    raise SkipTest('libdispofa is unavailable')


class DispofaTestCase(base.BaseTestCase):
    backend = 'dispofa'
    hosts = [os.envrion.get('YLOCK_TEST_DISPOFA_HOST', 'localhost')]
