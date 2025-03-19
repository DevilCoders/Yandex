import pytest
from schematics import types as basic_types

from yc_common import exceptions
from yc_common import models
from yc_common import validation
from yc_common.clients.kikimr.sql import QueryTemplate
from yc_common.clients.kikimr.sql import SqlIn
from yc_common.clients.kikimr.sql import SqlNotIn
from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import AbstractMktBase
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_filter
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_filter_with_category
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_order_by


class ExampleModel(AbstractMktBase):
    @property
    def PublicModel(self):
        return ExampleModel

    class Choices:
        A = "a"
        B = "b"
        C = "c"
        ALL = {A, B, C}

    id = validation.ResourceIdType()
    created_at = basic_types.IntType()
    name = basic_types.StringType()
    sand = basic_types.StringType()
    count = basic_types.IntType()
    time = models.IsoTimestampType()
    enum = models.StringEnumType(choices=Choices.ALL)

    Filterable_fields = {
        "name",
        "sand",
        "count",
        "time",
        "enum",
    }


def test_parse_filter_allowed_fields():
    with pytest.raises(exceptions.BadRequestError):
        parse_filter("unknown_field=2", ExampleModel)


@pytest.mark.parametrize("input", [
    "count='1'",  # number
    "name=123",  # string
    "time = 'foo'",  # timestamp,
    "enum=A",  # enum
])
def test_parse_filter_wrong_argument_type(input):
    with pytest.raises(exceptions.BadRequestError):
        parse_filter(input, ExampleModel)


@pytest.mark.parametrize("input_str,query,arg", [
    ("", [], []),
    ("name='test\\\\' and name='foo'", ["name = ? ", "name = ? "], ["test\\", "foo"]),  # string backslash before quote
    ("name='test\\\\\\'' and name='foo'", ["name = ? ", "name = ? "], ["test\\'", "foo"]),  # string slash before quote
    ("count=1", ["count = ? "], [1]),  # number
    ("name='test'", ["name = ? "], ["test"]),  # string single quote
    ("name='test\"\\''\"'", ["name = ? "], ["test\"''\""]),  # string backslash before quote
    ("name='test\\'s'", ["name = ? "], ["test's"]),  # string escaped single quote
    ("name=\"test\"", ["name = ? "], ["test"]),  # string double quote
    ("name=\"test\\\"s\"", ["name = ? "], ["test\"s"]),  # string escaped double quote
    ("time = 1528884603", ["time = ? "], [1528884603]),  # timestamp
    ("time =  2018-06-13T10:10:03", ["time = ? "], [1528884603]),  # iso8601
    ("enum = 'A'", ["enum = ? "], ["a"]),  # enum
])
def test_parse_filter(input_str: str, query, arg):
    filter_query, filter_args = parse_filter(input_str, ExampleModel)

    assert filter_query == query
    assert filter_args == arg


@pytest.mark.parametrize("input,arg", [
    ("name in ('test', 'another test')", SqlIn("name", ["test", "another test"])),  # list expression
    ("name IN ('test', 'another test')", SqlIn("name", ["test", "another test"])),  # list expression
    ("name NOT in ('test', 'another test')", SqlNotIn("name", ["test", "another test"])),  # list not expression
    ("name not IN ('test', 'another test')", SqlNotIn("name", ["test", "another test"])),  # list not expression
    ("name NOT IN ('test', 'another test')", SqlNotIn("name", ["test", "another test"])),  # list not expression
    ("name NOT IN ('test', 'in test')", SqlNotIn("name", ["test", "in test"])),  # list not expression
])
def test_parse_filter_list(input, arg):
    filter_query, filter_args = parse_filter(input, ExampleModel)

    assert filter_args[0].render(QueryTemplate()) == arg.render(QueryTemplate())


@pytest.mark.parametrize("input,groups", [
    ("count=1", 1),  # basic
    ("count=1 and name='test'", 2),  # lower case
    ("count=1 AND name='test'", 2),  # upper case
    ("count=1 AND sand='test'", 2),  # and substr in field name
    ("count=1 AND name in ('test', 'another test')", 2),  # list expression
    ("count=1 AND name='test and some more'", 2),  # quoted separator
])
def test_parse_filter_separator(input, groups):
    filter_query, filter_args = parse_filter(input, ExampleModel)

    assert len(filter_query) == groups
    assert len(filter_args) == groups


@pytest.mark.parametrize("input,groups,cid", [
    ("count=1", 1, None),  # basic
    # category is special
    ("count=1 and category_id='46ba74ec-77a3-42d3-ade6-6fa5a3954df7'", 1, "46ba74ec-77a3-42d3-ade6-6fa5a3954df7"),
    ("count=1 AND name='test'", 2, None),
])
def test_parse_filter_with_category(input, groups, cid):
    filter_query, filter_args, category_id = parse_filter_with_category(input, ExampleModel)

    assert len(filter_query) == groups
    assert len(filter_args) == groups
    assert category_id == cid


@pytest.mark.parametrize("query,res", [
    (None, "n.order ASC"),
    ("createdAt", "created_at ASC"),
    ("updatedAt DESC", "p.`updated_at` DESC"),
])
def test_parse_order_by(query, res):
    mapping = {
        "order": "n.order",
        "createdAt": "created_at",
        "updatedAt": "p.`updated_at`",
    }

    assert parse_order_by(query, mapping, "order") == res
