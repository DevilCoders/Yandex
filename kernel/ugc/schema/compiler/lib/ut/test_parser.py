from kernel.ugc.schema.compiler.lib import ParseException, SchemaParser
import pytest


#
# _parse_tokens() tests
#
def test_no_scol():
    with pytest.raises(ParseException):
        SchemaParser()._parse_tokens("a")


def test_no_expr():
    with pytest.raises(ParseException):
        SchemaParser()._parse_tokens(";")


def test_simple_expr():
    result = SchemaParser()._parse_tokens("a b c;")
    assert result == ["a", "b", "c"]


def test_complex_expr():
    result = SchemaParser()._parse_tokens("a { b; c; }")
    assert result == ["a", [["b"], ["c"]]]


def test_complex_expr_with_scol():
    result = SchemaParser()._parse_tokens("a { b; c; };")
    assert result == ["a", [["b"], ["c"]]]


def test_anon_list():
    with pytest.raises(ParseException):
        SchemaParser()._parse_tokens("{ a; }")


def test_expr_list():
    result = SchemaParser()._parse_tokens("l { a b c; d; }")
    assert result == ["l", [["a", "b", "c"], ["d"]]]


def test_nested_list():
    result = SchemaParser()._parse_tokens("l1 { l2 { a b c; } l3 { d; } }")
    assert result == ["l1", [["l2", [["a", "b", "c"]]], ["l3", [["d"]]]]]


#
# parse() tests
#
def test_valid_op():
    p = SchemaParser("KEY Key;")
    assert p.parse() == {"KEY": "Key"}


def test_invalid_op():
    p = SchemaParser("KEYYY Key;")
    with pytest.raises(ParseException):
        p.parse()


def test_schema():
    p = SchemaParser("""
        SCHEMA TestSchema {
        }
    """)
    assert p.parse() == {"SCHEMA": {"TestSchema": {}}}


def test_tables():
    p = SchemaParser("""
        TABLE Table1 {
            TABLE Table2 {
            }
            TABLE Table3 {
            }
        }
    """)
    assert p.parse() == {"TABLE": {"Table1": {"TABLE": {"Table2": {}, "Table3": {}}}}}


def test_key():
    p = SchemaParser("""
        TABLE Table {
            KEY Key;
        }
    """)
    assert p.parse() == {"TABLE": {"Table": {"KEY": "Key"}}}


def test_key_unique():
    p = SchemaParser("""
        TABLE Table {
            KEY Key1;
            KEY Key2;
        }
    """)
    with pytest.raises(ParseException):
        p.parse()


def test_row_unique():
    p = SchemaParser("""
        TABLE Table {
            ROW Row1 {}
            ROW Row2 {}
        }
    """)
    with pytest.raises(ParseException):
        p.parse()


def test_json_name_unique():
    p = SchemaParser("""
        TABLE Table {
            JSON_NAME table1;
            JSON_NAME table2;
        }
    """)
    with pytest.raises(ParseException):
        p.parse()


def test_noprefix():
    p = SchemaParser("""
        KEYSPACE test {
            NOPREFIX;
        }
    """)
    assert p.parse() == {"KEYSPACE": {"test": {"NOPREFIX": True}}}


def test_row_proto():
    p = SchemaParser("""
        ROW TRow {
            bool BoolField;
            string StringField;
            repeated string RepeatedField;
        }
    """)

    assert p.parse() == {
        "ROW": {
            "TRow": [
                ["bool", "BoolField"],
                ["string", "StringField"],
                ["repeated", "string", "RepeatedField"]
            ]
        }
    }
