#!/usr/bin/env python
# -*- coding: utf-8 -*-

import unittest

from templatetags.template_common import username

class UsernameTest(unittest.TestCase):
    def test_first_letter(self):
        self.assertEqual(username('gretel', autoescape = True), six.u('<b>g</b>retel'))
        self.assertEqual(username('gretel'), six.u('<b>g</b>retel'))

    def test_nonfirst_letter(self):
        self.assertEqual(username('(-g·r·e·t·e·l-)', autoescape = True), six.u('(-<b>g</b>·r·e·t·e·l-)'))
        self.assertEqual(username('(-g·r·e·t·e·l-)'), six.u('(-<b>g</b>·r·e·t·e·l-)'))

    def test_nonletter(self):
        self.assertEqual(username('(-*-)', autoescape = True), six.u('<b>(</b>-*-)'))
        self.assertEqual(username('(-*-)'), six.u('<b>(</b>-*-)'))

    def test_gt_sign(self):
        self.assertEqual(username('<gretel>', autoescape = True), six.u('&lt;<b>g</b>retel&gt;'))
        self.assertEqual(username('<gretel>'), six.u('<<b>g</b>retel>'))

    def test_russian(self):
        self.assertEqual(username('Гретель', autoescape = True), six.u('<b>Г</b>ретель'))
        self.assertEqual(username('Гретель'), six.u('<b>Г</b>ретель'))

    def test_empty(self):
        self.assertEqual(username(''), '')


if __name__ == '__main__':
    unittest.main()



