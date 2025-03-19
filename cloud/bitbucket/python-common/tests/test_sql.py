"""Tests SQL module"""
import decimal
import json
from uuid import UUID

import pytest
from yc_common.clients.kikimr.kikimr_types import KikimrDataType

from yc_common.clients.kikimr import SqlOrder
from yc_common.exceptions import LogicalError
from yc_common.clients.kikimr.exceptions import ValidationError
from yc_common.clients.kikimr.sql import build_query_template, _InvalidQueryError, SqlCompoundKeyMatch, SqlWhere, SqlIn, \
    SqlCompoundKeyCursor, SqlCompoundKeyOrderLimit, SqlInMany, SqlNotInMany, SqlNotIn, render_value, QueryTemplate, \
    _render_value_for_template, SqlCondition, substitute_variable, Variable
from yc_common.fields import StringType, IntType
from yc_common.models import Model


def _call_render(o) -> str:
    query_template = QueryTemplate()
    query_template.text_template = o.render(query_template)
    return query_template.build_text_query()


def test_prepare_query():
    assert build_query_template("SELECT * FROM some_table").build_text_query() == "SELECT * FROM some_table"
    assert build_query_template("SELECT * FROM some_table").build_text_query() == "SELECT * FROM some_table"

    with pytest.raises(_InvalidQueryError) as e:
        build_query_template("SELECT * FROM $")
    assert str(e.value) == "The query contains an unbound variable."

    assert build_query_template("SELECT * FROM $ydb_").build_text_query() == "SELECT * FROM $ydb_"

    assert build_query_template("SELECT $var1 FROM $var2", variables={
        "var1": "a",
        "var2": "b",
        "var3": "c",
    }).build_text_query() == "SELECT a FROM b"

    with pytest.raises(_InvalidQueryError) as e:
        build_query_template("SELECT * FROM some_table", 1)
    assert str(e.value) == "Invalid number of arguments is passed to the query."

    with pytest.raises(_InvalidQueryError) as e:
        build_query_template("SELECT * FROM some_table WHERE a = ?")
    assert str(e.value) == "Invalid number of arguments is passed to the query."

    assert build_query_template("SELECT * FROM some_table WHERE a = ?", 1).build_text_query() == "SELECT * FROM some_table WHERE a = 1"

    with pytest.raises(ValidationError) as e:
        assert build_query_template("SELECT * FROM some_table WHERE a = ?", object())
    assert str(e.value) == "Unsupported render value type: object."

    assert build_query_template("SELECT * FROM some_table WHERE a = ? AND b = ?", 1, "a b c").build_text_query() == \
        "SELECT * FROM some_table WHERE a = 1 AND b = 'a b c'"

    assert build_query_template("SELECT * FROM table WHERE flavor = ? AND scope=?", 1, 2).build_text_query() == \
        "SELECT * FROM table WHERE flavor = 1 AND scope=2"

    assert build_query_template("INSERT INTO table (v) VALUES(?)", Variable({}, KikimrDataType.UTF8)).build_text_query() == \
        "INSERT INTO table (v) VALUES('{}')"


def test_where_query_builder():
    query = SqlWhere()
    assert query.render() == ("", [])

    query.and_condition("a > 1")
    query.and_condition("b > ?", 2)
    query.and_condition("c > ?", 3)
    assert query.render() == ("WHERE a > 1 AND b > ? AND c > ?", [2, 3])

    in_query = SqlIn("d", [10, 20, 30])
    query.and_condition(in_query)
    assert query.render() == ("WHERE a > 1 AND b > ? AND c > ? AND ?", [2, 3, in_query])


def test_render_compound_key_match():
    result = _call_render(SqlCompoundKeyMatch(("key1", "key2", "key3"), [("1", "2", 3), ("3", "4", 5)]))
    assert result == "(key1 = '1' AND key2 = '2' AND key3 = 3 OR key1 = '3' AND key2 = '4' AND key3 = 5)"

    result = _call_render(SqlCompoundKeyMatch(("key1", "key2", "key3"), [("1", "2", None), (None, "4", 5)]))
    assert result == "(key1 = '1' AND key2 = '2' AND key3 IS NULL OR key1 IS NULL AND key2 = '4' AND key3 = 5)"

    result = _call_render(SqlCompoundKeyMatch(("key1", "key2", "key3"), [("1", "2", Variable(None, KikimrDataType.UTF8)), (Variable(None, KikimrDataType.INT64), "4", 5)]))
    assert result == "(key1 = '1' AND key2 = '2' AND key3 IS NULL OR key1 IS NULL AND key2 = '4' AND key3 = 5)"


@pytest.mark.parametrize("params", [(SqlInMany, "IN"), (SqlNotInMany, "NOT IN")])
def test_render_compound_key_match_in_many(params):
    conditioner, keyword = params
    query, args = conditioner(("key1", "key2", "key3"), [("1", "2", 3), ("3", "4", 5)]).render()
    assert query == "(key1, key2, key3) {} ((?, ?, ?), (?, ?, ?))".format(keyword)
    assert args == ["1", "2", 3, "3", "4", 5]


@pytest.mark.parametrize("conditioner", [SqlCompoundKeyMatch, SqlIn])
def test_render_sql_in_empty_values(conditioner):
    result = _call_render(conditioner((), []))
    assert result == "FALSE"


def test_render_sql_in_many_empty_values():
    result, args = SqlInMany((), []).render()
    assert result == "FALSE" and len(args) == 0


def test_render_sql_not_in_empty_values():
    result = _call_render(SqlNotIn((), []))
    assert result == "TRUE"


def test_render_sql_not_in_many_empty_values():
    result, args = SqlNotInMany((), []).render()
    assert result == "TRUE" and len(args) == 0


def test_render_sql_compaund_key_match_values_mismatch():
    with pytest.raises(LogicalError):
        SqlCompoundKeyMatch(("key1", "key2"), [("1", "2", "3"), ("3", "4", "5")]).render(QueryTemplate())


@pytest.mark.parametrize("conditioner", [SqlInMany, SqlNotInMany])
def test_render_sql_in_many_columns_values_mismatch(conditioner):
    with pytest.raises(LogicalError):
        conditioner(("key1", "key2"), [("1", "2", "3"), ("3", "4", "5")]).render()


def test_render_sql_order():
    assert _call_render(SqlOrder()) == ""
    assert _call_render(SqlOrder("asd")) == "ORDER BY asd ASC"
    assert _call_render(SqlOrder("+asd")) == "ORDER BY asd ASC"
    assert _call_render(SqlOrder("-asd")) == "ORDER BY asd DESC"
    assert _call_render(SqlOrder(["asd"])) == "ORDER BY asd ASC"
    assert _call_render(SqlOrder(["a", "b"])) == "ORDER BY a ASC, b ASC"
    assert _call_render(SqlOrder(["+a", "+b"])) == "ORDER BY a ASC, b ASC"
    assert _call_render(SqlOrder(["a", "-b"])) == "ORDER BY a ASC, b DESC"
    assert _call_render(SqlOrder(["-a", "b"])) == "ORDER BY a DESC, b ASC"
    assert _call_render(SqlOrder(["-a", "-b"])) == "ORDER BY a DESC, b DESC"


def test_render_value():
    # Simple
    assert render_value(None) == "NULL"
    assert render_value(True) == "true"
    assert render_value(False) == "false"
    assert render_value(123) == "123"
    assert render_value(123.0) == "123.0"
    assert render_value(UUID("12345678-1234-1234-1234-123456789abc")) == "'12345678-1234-1234-1234-123456789abc'"
    assert render_value("asd") == "'asd'"
    assert render_value(decimal.Decimal("1.123456789")) == "Decimal('1.123456789', 22, 9)"
    assert render_value(100500, KikimrDataType.UINT32) == "100500U"
    assert render_value(100500, KikimrDataType.UINT64) == "100500UL"

    with pytest.raises(ValidationError):
        render_value(bytes("asd", "utf-8"))

    with pytest.raises(ValidationError):
        render_value("100500;drop database", KikimrDataType.UINT32)
    with pytest.raises(ValidationError):
        render_value("100500;drop database", KikimrDataType.UINT64)

    # Escape
    assert render_value("as'd") == r"'as\'d'"


# noinspection PyProtectedMember
def test_render_for_prepare():
    def call_render_for_prepare(value):
        query_template = QueryTemplate()
        render_result = _render_value_for_template(value, query_template)
        assert render_result == "$ydb_auto_1"
        query_template.text_template = " $ydb_auto_1"
        return query_template.build_text_query()[1:]
    assert call_render_for_prepare(None) == "NULL"
    assert call_render_for_prepare(True) == "true"
    assert call_render_for_prepare(False) == "false"
    assert call_render_for_prepare(123) == "123"
    assert call_render_for_prepare(123.0) == "123.0"
    assert call_render_for_prepare(UUID("12345678-1234-1234-1234-123456789abc")) == "'12345678-1234-1234-1234-123456789abc'"
    assert call_render_for_prepare("asd") == "'asd'"

    with pytest.raises(ValidationError):
        render_value(bytes("asd", "utf-8"))

    # Escape
    assert render_value("as'd") == r"'as\'d'"

    assert call_render_for_prepare([1, 2, 3]) == "'[1, 2, 3]'"

    class ModelTest(Model):
        a = StringType()
        b = IntType()
    o = ModelTest()
    o.a = "asd"
    o.b = 123
    assert json.loads(call_render_for_prepare(o)[1:-1]) == json.loads("""'{"a": "asd", "b": 123}'"""[1:-1])

    # render renderable
    renderable = SqlOrder("asd")
    o1 = QueryTemplate()
    o2 = QueryTemplate()
    assert _render_value_for_template(renderable, o1) == renderable.render(o2) and o1.values == o2.values

    # render recursive renderable
    recursive_renderable = SqlCondition()
    recursive_renderable.and_condition("a > 2")
    o1 = QueryTemplate()
    o2 = QueryTemplate()
    q1 = _render_value_for_template(recursive_renderable, o1)
    q2, _ = recursive_renderable.render()
    assert q1 == q2 and o1.values == o2.values


# noinspection PyProtectedMember
def test_sql_order_reverse():
    assert SqlOrder().reverse()._fields is None
    assert SqlOrder([]).reverse()._fields is None
    assert SqlOrder(["a"]).reverse()._fields == ["-a"]
    assert SqlOrder(["a", "b", "c"]).reverse()._fields == ["-a", "-b", "-c"]
    assert SqlOrder(["-a", "b", "+c"]).reverse()._fields == ["a", "-b", "-c"]


def test_render_compound_cursor():
    assert SqlCompoundKeyCursor([], []).render() == ("", [])
    assert SqlCompoundKeyCursor(["k1"], [1]).render() == ("(k1 > ?)", [1])
    assert SqlCompoundKeyCursor(["k1", "k2"], [1, 2]).render() == ("(k1 > ? OR k1 = ? AND k2 > ?)", [1, 1, 2])
    assert SqlCompoundKeyCursor(["k1", "k2", "k3"], [1, 2, 3]).render() == ("(k1 > ? OR k1 = ? AND k2 > ? OR k1 = ? AND k2 = ? AND k3 > ?)", [1, 1, 2, 1, 2, 3])


def test_render_compound_order_limit():
    assert _call_render(SqlCompoundKeyOrderLimit([])) == ""
    assert _call_render(SqlCompoundKeyOrderLimit([], limit=5)) == "LIMIT 5UL"
    assert _call_render(SqlCompoundKeyOrderLimit(["key1"])) == "ORDER BY key1"
    assert _call_render(SqlCompoundKeyOrderLimit(["key1", "key2"], limit=7)) == "ORDER BY key1, key2 LIMIT 7UL"


def test_substitute_variable():
    assert substitute_variable(" $a ", "a", "") == "  "
    assert substitute_variable(" $a ", "a", "asd") == " asd "
    assert substitute_variable(" $a ", "a", "'asd'") == " 'asd' "
    assert substitute_variable(" $a ", "a", r"'a\sd'") == r" 'a\sd' "
    assert substitute_variable(" $a ", "a", r"'a\\sd'") == r" 'a\\sd' "
