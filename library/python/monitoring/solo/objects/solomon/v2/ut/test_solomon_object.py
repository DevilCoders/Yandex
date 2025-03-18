# -*- coding: utf-8 -*-
import copy
import logging

import jsonobject

from library.python.monitoring.solo.objects.solomon.v2.base import SolomonObject


class A(SolomonObject):
    string_field = jsonobject.StringProperty(name="stringField", default="default")
    integer_field = jsonobject.IntegerProperty(name="integerField", default=1)


class B(SolomonObject):
    sorted_list = jsonobject.ListProperty(name="sortedList", item_type=A)
    unsorted_list = jsonobject.SetProperty(name="unsortedList", item_type=A)
    object = jsonobject.ObjectProperty(item_type=A, default=None, exclude_if_none=True)


class TestJSONObject(object):

    def test_basic_usage(self):
        x = B(
            sorted_list=[
                A(string_field="1", integer_field=1),
                A(string_field="2", integer_field=2),
                A(string_field="3", integer_field=3),
            ],
            unsorted_list={
                A(string_field="4", integer_field=4),
                A(string_field="5", integer_field=5),
                A(string_field="6", integer_field=6),
            },
            object=A(string_field="7", integer_field=7)
        )
        logging.error(type(x.sorted_list))
        assert x.sorted_list == x["sortedList"]

    def test_equality(self):
        x = A(string_field="1", integer_field=1)
        y = A(string_field="1", integer_field=1)
        assert id(x) != id(y)
        assert x == y
        y.string_field = "2"
        assert x != y

    def __run_copy_test(self, copy_function):
        b = B(
            sorted_list=[
                A(string_field="1", integer_field=1),
                A(string_field="2", integer_field=2),
                A(string_field="3", integer_field=3),
            ],
            unsorted_list={
                A(string_field="4", integer_field=4),
                A(string_field="5", integer_field=5),
                A(string_field="6", integer_field=6),
            },
            object=A(string_field="7", integer_field=7)
        )
        b_copy = copy_function(b)

        assert id(b) != id(b_copy)

        assert id(b.sorted_list) != id(b_copy.sorted_list)
        assert id(b.unsorted_list) != id(b_copy.unsorted_list)
        assert id(b.object) != id(b_copy.object)

        assert id(b.sorted_list[0]) != id(b_copy.sorted_list[0])
        assert id(b.unsorted_list) != id(b_copy.unsorted_list)

        assert b.sorted_list[0].string_field == b_copy.sorted_list[0].string_field
        b.sorted_list[0].string_field = "100500"
        assert b.sorted_list[0].string_field != b_copy.sorted_list[0].string_field

    def test_copy_copy(self):
        # "copy.copy" copies "copy.deepcopy" behavior for jsonobject
        self.__run_copy_test(copy.copy)

    def test_copy_deepcopy(self):
        self.__run_copy_test(copy.deepcopy)

    # TODO(lazuka23): add tests

    # def test_drop_fields(self):
    #     assert True

    # def test_from_dict(self):
    #     assert True

    # def test_to_dict(self):
    #     assert True
