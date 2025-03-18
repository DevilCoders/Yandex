import pytest

import tools.merge_json as merge_json


# noinspection PyClassHasNoInit
class TestUpdate:
    def test_dict(self):
        json1 = {
            "a": 123,
            "b": "hello world",
            "c": "c",
        }
        json2 = {
            "a": 123,
            "b": "hello world",
            "d": "d",
        }
        dst = {}
        merge_json.update(dst, json1)
        assert dst == json1
        merge_json.update(dst, json2)
        assert dst == {
            "a": 123,
            "b": "hello world",
            "c": "c",
            "d": "d",
        }

    def test_list(self):
        json1 = [
            123,
            "hello world",
            "c",
        ]
        json2 = [
            "d",
            "e",
            [1, 2, 3],
        ]
        dst = []
        merge_json.update(dst, json1)
        assert dst == json1
        merge_json.update(dst, json2)
        assert dst == json1 + json2

    def test_bad(self):
        json1 = {
            "a": 123,
            "b": 1,
        }
        json2 = {
            "a": 123,
            "b": 2,
        }
        with pytest.raises(Exception):
            merge_json.update(json1, json2)


# noinspection PyClassHasNoInit
class TestMerge:
    def test_single(self):
        json1 = {
            "a": 123,
            "b": "hello world",
            "c": "c",
        }
        assert merge_json.merge([json1]) == json1
        json2 = [
            "d",
            "e",
            [1, 2, 3],
        ]
        assert merge_json.merge([json2]) == json2
        json3 = "hello world"
        assert merge_json.merge([json3]) == json3
        json4 = None
        assert merge_json.merge([json4]) == json4

    def test_dict(self):
        json1 = {
            "a": 123,
            "b": "hello world",
            "c": "c",
        }
        json2 = {
            "a": 123,
            "b": "hello world",
            "d": "d",
        }
        merged = merge_json.merge([json1, json2])
        assert merged == {
            "a": 123,
            "b": "hello world",
            "c": "c",
            "d": "d",
        }

    def test_list(self):
        json1 = [
            123,
            "hello world",
            "c",
        ]
        json2 = [
            "d",
            "e",
            [1, 2, 3],
        ]
        merged = merge_json.merge([json1, json2])
        assert merged == json1 + json2

    def test_empty(self):
        assert merge_json.merge([]) is None

    def test_bad(self):
        with pytest.raises(Exception):
            merge_json.merge([[], {}])
        with pytest.raises(Exception):
            merge_json.merge([{"x": 1}, {"x": 2}])
