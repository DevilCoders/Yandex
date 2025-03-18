# coding=utf-8
import pytest
import re2

from antiadblock.cryprox.cryprox.common.tools.jsonify_meta import jsonify_meta
from antiadblock.cryprox.cryprox.config.bk import META_JS_OBJ_EXTRACTOR_RE


META_JS_OBJ_EXTRACTOR_RE_COMPILED = re2.compile('^' + META_JS_OBJ_EXTRACTOR_RE + '$', re2.IGNORECASE)


@pytest.mark.parametrize('meta_before, meta_after', [
    ("""Ya [1524124928839]('{ "rtb": [1,,2], "str": "123" }')""", {'rtb': [1, 2], 'str': '123'}),
    ("""ololoShKa [1524124928839]('{ "rtb": [1,,2], "str": "123" }')""", {'rtb': [1, 2], 'str': '123'}),
    ("""Ya [1524124928839]('{ "rtb": [1,,2], str: "123" }')""", {'rtb': [1, 2], 'str': '123'}),
    ("""Ya [1524124928839]('{ "rtb": [1,,2], str: "123", }')""", {'rtb': [1, 2], 'str': '123'}),
    ("""Ya [1524124928839]('{ "rtb": [1,,2], 'str': "123" }')""", {'rtb': [1, 2], 'str': '123'}),
    ("""Ya [1524124928839]('{ "rtb": [1,2], 'str': 'val: "123"'}')""", {'rtb': [1, 2], 'str': 'val: "123"'}),
    ("""Ya [1524124928839]('{ "rtb": [1,2], "params": "eta: 21 июня, 16:00\\/;\\/warranty: 3 года" }')""", {'rtb': [1, 2], 'params': u'eta: 21 июня, 16:00/;/warranty: 3 года'}),
    ("""Ya [1524124928839]('{ "rtb": [1,,,,2], ms: [,,,], 'str': "123" }')""", {'rtb': [1, 2], 'ms': [], 'str': '123'}),
    ("""Ya [1524124928839]('{ "rtb": [1,,,,2], ms: {}, 'str': "123" }')""", {'rtb': [1, 2], 'ms': {}, 'str': '123'}),
    ("""Ya [1524124928839]('{ ms:{key: 1}, 'str': "123" }')""", {'ms': {'key': 1}, 'str': '123'}),
    ("""Ya [1524124928839]('{ ms:{key:{key2:{key3:1}}}, 'str': "123" }')""", {'ms': {'key': {'key2': {'key3': 1}}}, 'str': '123'}),
    ("""Ya [1524124928839]('{ "rtb": [1,,,2,], \n'str': "123" }')""", {'rtb': [1, 2], 'str': '123'}),
    (""" Ya  [1524124928839]('{ "rtb": [1,,2], 'str': "123" }')""", {'rtb': [1, 2], 'str': '123'}),
    ("""/**/ Ya [1524124928839]('{ "rtb": [1,,2], \'str\': "123" }')""", {'rtb': [1, 2], 'str': '123'}),
    ("""Ya [1524124928839]('{ "rtb": ["русский",,"с пропуском"], bl: false}')""", {'rtb': [u'русский', u'с пропуском'], 'bl': False}),
    ("""Ya [1524124928839]('{ "str": "otkryt\\'_demo_schet" }')""", {"str": "otkryt'_demo_schet"}),
    ("""Ya [1524124928839]('{ "str": "otkryt\\\\'_demo_schet" }')""", {"str": "otkryt\\'_demo_schet"}),
    # проверим, что не ломаем json с одинарной кавычкой внутри текста
    ("""Ya [1524124928839]('{ "str": "otkryt'_demo_schet" }')""", {"str": "otkryt'_demo_schet"}),
    ("""Ya [1524124928839]('{ "text": {"params":"напряжение, v: 220/;/мощность лампы, w: 15/;/диаметр, мм: 505"} }')""",
     {"text": {"params": u"напряжение, v: 220/;/мощность лампы, w: 15/;/диаметр, мм: 505"}}),
])
def test_jsonify_meta(meta_before, meta_after):

    jsonified = jsonify_meta(meta_before, META_JS_OBJ_EXTRACTOR_RE_COMPILED)
    assert jsonified == meta_after


def test_jsonify_not_valid_meta_response():
    with pytest.raises(ValueError):
        jsonify_meta('Ya [1524124928839](\'{ "not a valid RTB Response"}\')', META_JS_OBJ_EXTRACTOR_RE_COMPILED)
