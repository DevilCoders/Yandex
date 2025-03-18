#!/usr/bin/env python

from tools.ygetparam.curly_braces_expander import Expand, IsExpandable, ExpandToStr

from devtools.fleur.ytest import suite, test, generator, TestSuite
from devtools.fleur.ytest.tools import AssertEqual, AssertRaises


@suite
class CurlyBracesExpand(TestSuite):

    @test
    def NotExpandable(self):
        var = 'asdqwe'
        result = ('asdqwe',)
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleDigit(self):
        var = '{0..5}'
        result = ('0', '1', '2', '3', '4', '5')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleReverseDigit(self):
        var = '{5..0}'
        result = ('5', '4', '3', '2', '1', '0')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleNegativeDigitsIncrease(self):
        var = '{-10..-5}'
        result = ('-10', '-9', '-8', '-7', '-6', '-5')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleNegativeDigitsDecrease(self):
        var = '{-10..-15}'
        result = ('-10', '-11', '-12', '-13', '-14', '-15')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleNegativeAndPositiveDigitsIncrease(self):
        var = '{-10..5}'
        result = ('-10', '-9', '-8', '-7', '-6', '-5', '-4', '-3', '-2',
                  '-1', '0', '1', '2', '3', '4', '5')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleNegativeAndPositiveDigitsIncreaseWithStep(self):
        var = '{-10..5..2}'
        result = ('-10', '-8', '-6', '-4', '-2',
                  '0', '2', '4')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleNegativeAndPositiveDigitsIncreaseWithZeroes(self):
        # BASH 4 incopatible
        var = '{-10..05}'
        result = ('-10', '-9', '-8', '-7', '-6', '-5', '-4', '-3', '-2',
                  '-1', '00', '01', '02', '03', '04', '05')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleNegativeAndPositiveDigitsDecrease(self):
        var = '{10..-5}'
        result = ('10', '9', '8', '7', '6', '5', '4', '3', '2', '1', '0',
                  '-1', '-2', '-3', '-4', '-5')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleNegativeAndPositiveDigitsDecreaseWithStep(self):
        var = '{10..-5..2}'
        result = ('10', '8', '6', '4', '2', '0',
                  '-2', '-4')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleNegativeAndPositiveDigitsDecreaseWithZeroes(self):
        # BASH 4 incopatible
        var = '{10..-05}'
        result = ('10', '09', '08', '07', '06', '05', '04', '03', '02',
                  '01', '00', '-1', '-2', '-3', '-4', '-5')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleDigitWithStep(self):
        var = '{0..5..3}'
        result = ('0', '3')
        AssertEqual(Expand(var), result)

    @test
    def ExpandMultipleDigits(self):
        var = '{00..55..3}'
        result = ('00', '03', '06', '09', '12', '15', '18', '21', '24', '27',
                  '30', '33', '36', '39', '42', '45', '48', '51', '54')
        AssertEqual(Expand(var), result)

    @test
    def ExpandMixedLengthDigits(self):
        var = '{0..55..3}'
        result = ('0', '3', '6', '9', '12', '15', '18', '21', '24', '27',
                  '30', '33', '36', '39', '42', '45', '48', '51', '54')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleChar(self):
        var = '{a..d}'
        result = ('a', 'b', 'c', 'd')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleReverseChar(self):
        var = '{d..a}'
        result = ('d', 'c', 'b', 'a')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleCharWithStep(self):
        var = '{a..d..2}'
        result = ('a', 'c')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleCharWithRedefinedStep(self):
        var = '{a..d}'
        result = ('a', 'c')
        AssertEqual(Expand(var, step=2), result)

    @test
    def ExpandMultipleBraces(self):
        var = '{a..d}{1..5}'
        result = ('a1', 'a2', 'a3', 'a4', 'a5', 'b1', 'b2', 'b3', 'b4', 'b5',
                  'c1', 'c2', 'c3', 'c4', 'c5', 'd1', 'd2', 'd3', 'd4', 'd5')
        AssertEqual(Expand(var), result)

    @test
    def ExpandMultipleBracesWithPre(self):
        var = 'T{a..d}{1..5}'
        result = ('Ta1', 'Ta2', 'Ta3', 'Ta4', 'Ta5', 'Tb1', 'Tb2', 'Tb3',
                  'Tb4', 'Tb5', 'Tc1', 'Tc2', 'Tc3', 'Tc4', 'Tc5', 'Td1',
                  'Td2', 'Td3', 'Td4', 'Td5')
        AssertEqual(Expand(var), result)

    @test
    def ExpandMultipleBracesWithPost(self):
        var = '{a..d}{1..5}T'
        result = ('a1T', 'a2T', 'a3T', 'a4T', 'a5T', 'b1T', 'b2T', 'b3T',
                  'b4T', 'b5T', 'c1T', 'c2T', 'c3T', 'c4T', 'c5T', 'd1T',
                  'd2T', 'd3T', 'd4T', 'd5T')
        AssertEqual(Expand(var), result)

    @test
    def ExpandMultipleBracesWithMiddle(self):
        var = '{a..d}T{1..5}'
        result = ('aT1', 'aT2', 'aT3', 'aT4', 'aT5', 'bT1', 'bT2', 'bT3',
                  'bT4', 'bT5', 'cT1', 'cT2', 'cT3', 'cT4', 'cT5', 'dT1',
                  'dT2', 'dT3', 'dT4', 'dT5')
        AssertEqual(Expand(var), result)

    @test
    def ExpandMultipleBracesWithiPreMiddlePost(self):
        var = 'I{a..d}J{1..5}K'
        result = ('IaJ1K', 'IaJ2K', 'IaJ3K', 'IaJ4K', 'IaJ5K', 'IbJ1K',
                  'IbJ2K', 'IbJ3K', 'IbJ4K', 'IbJ5K', 'IcJ1K', 'IcJ2K',
                  'IcJ3K', 'IcJ4K', 'IcJ5K', 'IdJ1K', 'IdJ2K', 'IdJ3K',
                  'IdJ4K', 'IdJ5K')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleComma(self):
        var = '{a,b}'
        result = ('a', 'b')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleValuedComma(self):
        var = '{,b}'
        result = ('', 'b')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleValuedCommaWithPrePost(self):
        var = 'a{,b}c'
        result = ('ac', 'abc')
        AssertEqual(Expand(var), result)

    @test
    def ExpandSingleEmptyCommaWithPrePost(self):
        var = 'a{,}c'
        result = ('ac', 'ac')
        AssertEqual(Expand(var), result)

    @test
    def ExpandComplexDoubleDotsWithCommas(self):
        var = '{{a..d},{0..5},_}'
        result = ('a', 'b', 'c', 'd', '0', '1', '2', '3', '4', '5', '_')
        AssertEqual(Expand(var), result)

    @test
    def ExpandComplexDoubleDotsWithCommas2(self):
        var = '{{a.d},{0..5},T}'
        result = ('{a.d}', '0', '1', '2', '3', '4', '5', 'T')
        AssertEqual(Expand(var), result)

    @test
    def ExpandComplexDoubleDotsWithCommas3(self):
        var = '{{cold{1..3},wspm}.search.yandex.net,wspm3.yandex.ru}'
        result = ('cold1.search.yandex.net', 'cold2.search.yandex.net',
                  'cold3.search.yandex.net', 'wspm.search.yandex.net',
                  'wspm3.yandex.ru')
        AssertEqual(Expand(var), result)

    @test
    def ExpandComplexDoubleDotsWithCommas4(self):
        var = '{wspm3.yandex.ru,{cold{1..3},wspm}.search.yandex.net}'
        result = ('wspm3.yandex.ru', 'cold1.search.yandex.net',
                  'cold2.search.yandex.net', 'cold3.search.yandex.net',
                  'wspm.search.yandex.net')
        AssertEqual(Expand(var), result)

    @test
    def ExpandComplexDoubleDotsWithCommas5(self):
        var = '{wspm3.yandex.ru,text{cold{1..3},wspm}}'
        result = ('wspm3.yandex.ru', 'textcold1', 'textcold2',
                  'textcold3', 'textwspm')
        AssertEqual(Expand(var), result)

    @test
    def ExpandComplexMissbalanced1(self):
        var = '{{a..d},{0..5,T}'
        result = ('{a,0..5', '{a,T', '{b,0..5', '{b,T', '{c,0..5', '{c,T',
                  '{d,0..5', '{d,T')
        AssertEqual(Expand(var), result)

    @test
    def ExpandComplexMissbalanced2(self):
        var = '{a..d},{0..5},T}'
        result = ('a,0,T}', 'a,1,T}', 'a,2,T}', 'a,3,T}', 'a,4,T}', 'a,5,T}',
                  'b,0,T}', 'b,1,T}', 'b,2,T}', 'b,3,T}', 'b,4,T}', 'b,5,T}',
                  'c,0,T}', 'c,1,T}', 'c,2,T}', 'c,3,T}', 'c,4,T}', 'c,5,T}',
                  'd,0,T}', 'd,1,T}', 'd,2,T}', 'd,3,T}', 'd,4,T}', 'd,5,T}')
        AssertEqual(Expand(var), result)

    @test
    def ExpandIncorrectRanges1(self):
        var = '{a..1}'
        result = ('{a..1}',)
        AssertEqual(Expand(var), result)

    @test
    def ExpandIncorrectRanges2(self):
        var = '{a..d..a}'
        result = ('{a..d..a}',)
        AssertEqual(Expand(var), result)

    @test
    def ExpandIncorrectRanges3(self):
        var = '{a..d..1..3}'
        result = ('{a..d..1..3}',)
        AssertEqual(Expand(var), result)


@suite
class CurlyBracesIsExpandable(TestSuite):

    @test
    def ExpandableOrNot(self):
        AssertEqual(IsExpandable('asdqwe'), False)
        AssertEqual(IsExpandable('{{z..a},'), True)
        AssertEqual(IsExpandable('{{a..d},{0..5,T}'), True)
        AssertEqual(IsExpandable('{a..d},{0..5,T}}'), True)
        AssertEqual(IsExpandable('{{a..d},{0..5},_}'), True)


@suite
class CurlyBracesExpandToString(TestSuite):

    @test
    def ExpandUnexpandable(self):
        AssertEqual(ExpandToStr('asdqwe'), 'asdqwe')

    @test
    def ExpandUnexpandableTwoWords(self):
        AssertEqual(ExpandToStr('asd qwe'), 'asd qwe')

    @test
    def ExpandStringAndSingleExpandable(self):
        var = 'a {1..5}'
        result = 'a 1 2 3 4 5'
        AssertEqual(ExpandToStr(var), result)

    @test
    def ExpandStringAndDoubleExpandables(self):
        var = 'a {1..5} {a..c}'
        result = 'a 1 2 3 4 5 a b c'
        AssertEqual(ExpandToStr(var), result)

    @test
    def ExpandStringAndDoubleExpandablesWithRedefinedDelimiter(self):
        var = 'a:{1..5}:{a..c}'
        result = 'a:1:2:3:4:5:a:b:c'
        AssertEqual(ExpandToStr(var, delim=':'), result)

    @test
    def ExpandStringAndDoubleExpandablesWithoutDelimiter1(self):
        var = 'a {1..3} {a..c}'
        result = 'a 1 aa 1 ba 1 ca 2 aa 2 ba 2 ca 3 aa 3 ba 3 c'
        AssertEqual(ExpandToStr(var, delim=''), result)

    @test
    def ExpandStringAndDoubleExpandablesWithoutDelimiter2(self):
        var = 'a:{1..3}:{a..c}'
        result = 'a:1:aa:1:ba:1:ca:2:aa:2:ba:2:ca:3:aa:3:ba:3:c'
        AssertEqual(ExpandToStr(var, delim=''), result)

    @test
    def ExpandStringAndDoubleExpandablesAndDouelSpaces(self):
        var = 'ls -la  file{1,{4..6}}.jpg |tee fileN.log'
        result = 'ls -la  file1.jpg file4.jpg file5.jpg file6.jpg |tee fileN.log'
        AssertEqual(ExpandToStr(var), result)

    @test
    def ExpandStringAndDoubleExpandablesWithLongDelimiter(self):
        var = 'a{1..3}.ya.ru'
        result = 'a1.ya.ru, a2.ya.ru, a3.ya.ru'
        AssertEqual(ExpandToStr(var, delim=', '), result)
