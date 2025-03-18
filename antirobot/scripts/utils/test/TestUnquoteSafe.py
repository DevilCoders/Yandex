# -*- coding: utf-8 -*-

from devtools.fleur.ytest import suite, TestSuite, generator
from devtools.fleur.ytest import AssertEqual, AssertEqual

from antirobot.scripts.utils import unquote_safe
import types

@suite(package="antirobot.scripts.utils")
class UnquoteSafe(TestSuite):
    @generator([
        (u'значение', 'значение'),
        (u'Строка UTF-8', '%D0%A1%D1%82%D1%80%D0%BE%D0%BA%D0%B0%20UTF-8'),
        (u'Кодировка windows я', '%cA%eE%E4%E8%f0%EE%E2%Ea%e0%20windows%20%Ff'),
        (ur'\xc3\x83\xc6\x92\xc3\x86\xe2\x80\x99', r'\xc3\x83\xc6\x92\xc3\x86\xe2\x80\x99'),
        ])
    def Unquote(self, expectVal, inputVal):
        result = unquote_safe.SafeUnquote(inputVal)

        AssertEqual(types.UnicodeType, type(result))
        AssertEqual(expectVal, result)
