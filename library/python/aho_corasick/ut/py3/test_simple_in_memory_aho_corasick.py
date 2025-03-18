# -*- coding: utf-8 -*-

from library.python.aho_corasick import SimpleInMemoryAhoCorasick


def test_simple_in_memory_aho_corasick():
    strings = [
        u'съешь еще',
        u'ещё ',
        u' этих ',
        u' мягк',
        'lorem',
        u'ore',
        ' ipsu',
        '42',
        u'два',
        u'два',
    ]
    aho_corasick = SimpleInMemoryAhoCorasick(strings)

    assert sorted(list(aho_corasick.find(u'съешь еще этих'))) == [u'съешь еще']
    assert sorted(list(aho_corasick.find(u'съешь ещё этих '))) == [u' этих ', u'ещё ']
    assert sorted(list(aho_corasick.find(u'этих мягких'))) == [u' мягк']
    assert sorted(list(aho_corasick.find(u'съешь еще lorem ipsum'))) == [' ipsu', 'lorem', u'ore', u'съешь еще']
    assert sorted(list(aho_corasick.find(u'ещё 42'))) == ['42', u'ещё ']
    assert sorted(list(aho_corasick.find('42424247'))) == ['42', '42', '42']
    assert sorted(list(aho_corasick.find(u'абырвалг'))) == []
    assert sorted(list(aho_corasick.find(u'раз-два-три'))) == [u'два', u'два']
