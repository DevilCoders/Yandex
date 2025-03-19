# -*- coding: utf-8 -*-

import json
import pytest
import six

from kernel.facts.dynamic_list_replacer_pylib import (
    match_fact_text_with_list_candidate,
    convert_list_data_to_rich_fact,
    normalize_text_for_indexer,
    dynamic_list_replacer_hash_vector,
    )


FACT_TEXT = u'К таким привычкам относятся: сидеть или ходить, сгорбив спину; привычка грызть ноги...'

LIST_CANDIDATE_INDEX_JSON = u'''
    {
        "body_hash": "092AnRa8ttEnOKe2bsda5cALa8Sf/+Lzh/zuYmAZBLCJYYuVpSdRKRDa96WiBwbN7mIjnZD/UWEhatTbnKtI8BKdKbKIi0YbXPOHEr/mEk4+zV8yS8G/5i0YXfkGLGxF7jZIM1B2aI9t+d0Dv+bQx4cqUx6iBwHNXZY=",
        "header_lens": [29, 4],
        "items_lens": [5, 3, 2, 5, 1, 1, 1, 3, 7]
    }
    '''


SERP_DATA_IN_JSON = u'''
    {
        "source": "fact_snippet",
        "type": "suggest_fact",
        "url": "http://03hm.ru/info/news/priroda-vozniknoveniya-vrednykh-privychek/"
    }
    '''

LIST_DATA_JSON = u'''
    {
        "header": [
            "Вредные привычки имеют также обширный перечень, который знаком каждому с ранних лет, \
потому что очень часто повторяют мамы, бабушки, воспитатели детского сада и учителя, что нельзя делать некоторые вещи.",
            "К таким привычкам относятся:"
        ],
        "items": [
            "сидеть или ходить, сгорбив спину",
            "привычка грызть ноги",
            "«щелканье» суставами",
            "привычка питаться едой быстрого приготовления",
            "курение",
            "алкоголь",
            "наркотики",
            "неумеренность в еде",
            "привычка поздно ложиться спать"
        ],
        "type": 0
    }
    '''

SERP_DATA_EXPECTED_JSON_A = u'''
    {
    '''

SERP_DATA_EXPECTED_JSON_B = u'''
        "source": "fact_snippet",
        "url": "http://03hm.ru/info/news/priroda-vozniknoveniya-vrednykh-privychek/",
    '''

SERP_DATA_EXPECTED_JSON_C = u'''
        "type": "rich_fact",
        "use_this_type": 1,
        "visible_items": [
            {
                "content": [
                    {
                        "type": "text",
                        "text": "Вредные привычки имеют также обширный перечень, который знаком каждому с ранних лет, \
потому что очень часто повторяют мамы, бабушки, воспитатели детского сада и учителя, что нельзя делать некоторые вещи. К таким привычкам относятся:"
                    }
                ]
            },
            {
                "marker": "bullet",
                "content": [
                    {
                        "type": "text",
                        "text": "сидеть или ходить, сгорбив спину"
                    }
                ]
            },
            {
                "marker": "bullet",
                "content": [
                    {
                        "type": "text",
                        "text": "привычка грызть ноги"
                    }
                ]
            },
            {
                "marker": "bullet",
                "content": [
                    {
                        "type": "text",
                        "text": "«щелканье» суставами"
                    }
                ]
            },
            {
                "marker": "bullet",
                "content": [
                    {
                        "type": "text",
                        "text": "привычка питаться едой быстрого приготовления"
                    }
                ]
            },
            {
                "marker": "bullet",
                "content": [
                    {
                        "type": "text",
                        "text": "курение"
                    }
                ]
            },
            {
                "marker": "bullet",
                "content": [
                    {
                        "type": "text",
                        "text": "алкоголь"
                    }
                ]
            },
            {
                "marker": "bullet",
                "content": [
                    {
                        "type": "text",
                        "text": "наркотики"
                    }
                ]
            },
            {
                "marker": "bullet",
                "content": [
                    {
                        "type": "text",
                        "text": "неумеренность в еде"
                    }
                ]
            },
            {
                "marker": "bullet",
                "content": [
                    {
                        "type": "text",
                        "text": "привычка поздно ложиться спать"
                    }
                ]
            }
        ]
    }
    '''


TEXT = u'  Какой-нибудь  текст (со скобочками) и ударе\u0301нием, и пере\u00ADносом, и буквой Ёё, и смайликом ☺, и иероглифом 呆 -- ' + \
       u'чтобы !!!ПРОВЕРИТЬ!!!, \n\r А) \r\n Б) \n В) \r Г) ... \"\' как он тут <{[123.45]}> нормализуется...    '

NORM = u'какой нибудь текст со скобочками и ударением и переносом и буквой ее и смайликом и иероглифом 呆 чтобы проверить а б в г как он тут 123 45 нормализуется'

HASH = [10249, 21191, 2600, 50699, 7100, 1954, 18973, 1954, 13821, 1954, 37287, 51437, 1954, 38517, 1954, 58543, 26659,
        5314, 5281, 59977, 61965, 63853, 45466, 29369, 57142, 12769, 13386, 57494, 10696, ]


def test_match_fact_text_with_list_candidate_text():
    match_result = match_fact_text_with_list_candidate(dynamic_list_replacer_hash_vector(FACT_TEXT), LIST_CANDIDATE_INDEX_JSON)
    assert match_result.list_candidate_score == 12
    assert match_result.number_of_header_elements_to_skip == 1


def test_match_fact_text_with_list_candidate_binary():
    match_result = match_fact_text_with_list_candidate(dynamic_list_replacer_hash_vector(FACT_TEXT), LIST_CANDIDATE_INDEX_JSON.encode('utf-8'))
    assert match_result.list_candidate_score == 12
    assert match_result.number_of_header_elements_to_skip == 1


def test_match_fact_text_with_list_candidate_none():
    with pytest.raises(TypeError):
        match_fact_text_with_list_candidate(HASH, None)


def test_convert_list_data_to_rich_fact_text_text():
    serp_data_out_json = convert_list_data_to_rich_fact(SERP_DATA_IN_JSON, LIST_DATA_JSON, 0)
    assert isinstance(serp_data_out_json, six.text_type)
    assert json.loads(serp_data_out_json) == json.loads(SERP_DATA_EXPECTED_JSON_A + SERP_DATA_EXPECTED_JSON_B + SERP_DATA_EXPECTED_JSON_C)


def test_convert_list_data_to_rich_fact_text_binary():
    serp_data_out_json = convert_list_data_to_rich_fact(SERP_DATA_IN_JSON, LIST_DATA_JSON.encode('utf-8'), 0)
    assert isinstance(serp_data_out_json, six.text_type)
    assert json.loads(serp_data_out_json) == json.loads(SERP_DATA_EXPECTED_JSON_A + SERP_DATA_EXPECTED_JSON_B + SERP_DATA_EXPECTED_JSON_C)


def test_convert_list_data_to_rich_fact_binary_text():
    serp_data_out_json = convert_list_data_to_rich_fact(SERP_DATA_IN_JSON.encode('utf-8'), LIST_DATA_JSON, 0)
    assert isinstance(serp_data_out_json, six.binary_type)
    assert json.loads(serp_data_out_json) == json.loads((SERP_DATA_EXPECTED_JSON_A + SERP_DATA_EXPECTED_JSON_B + SERP_DATA_EXPECTED_JSON_C).encode('utf-8'))


def test_convert_list_data_to_rich_fact_binary_binary():
    serp_data_out_json = convert_list_data_to_rich_fact(SERP_DATA_IN_JSON.encode('utf-8'), LIST_DATA_JSON.encode('utf-8'), 0)
    assert isinstance(serp_data_out_json, six.binary_type)
    assert json.loads(serp_data_out_json) == json.loads((SERP_DATA_EXPECTED_JSON_A + SERP_DATA_EXPECTED_JSON_B + SERP_DATA_EXPECTED_JSON_C).encode('utf-8'))


def test_convert_list_data_to_rich_fact_none_text():
    serp_data_out_json = convert_list_data_to_rich_fact(None, LIST_DATA_JSON, 0)
    assert isinstance(serp_data_out_json, six.text_type)
    assert json.loads(serp_data_out_json) == json.loads(SERP_DATA_EXPECTED_JSON_A + SERP_DATA_EXPECTED_JSON_C)


def test_convert_list_data_to_rich_fact_text_none():
    with pytest.raises(TypeError):
        convert_list_data_to_rich_fact(SERP_DATA_IN_JSON, None, 0)


def test_normalize_text_for_indexer_arg_text():
    assert normalize_text_for_indexer(TEXT) == NORM


def test_normalize_text_for_indexer_arg_binary():
    assert normalize_text_for_indexer(TEXT.encode('utf-8')) == NORM.encode('utf-8')


def test_normalize_text_for_indexer_arg_none():
    with pytest.raises(TypeError):
        normalize_text_for_indexer(None)


def test_dynamic_list_replacer_hash_vector_arg_text():
    assert dynamic_list_replacer_hash_vector(TEXT) == HASH


def test_dynamic_list_replacer_hash_vector_arg_binary():
    assert dynamic_list_replacer_hash_vector(TEXT.encode('utf-8')) == HASH


def test_dynamic_list_replacer_hash_vector_arg_none():
    with pytest.raises(TypeError):
        dynamic_list_replacer_hash_vector(None)
