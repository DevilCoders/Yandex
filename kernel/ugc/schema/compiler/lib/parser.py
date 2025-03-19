from collections import OrderedDict
from collections.abc import Mapping
from pyparsing import Forward, Group, OneOrMore, Optional, ParseException, Suppress, Word, ZeroOrMore, alphanums


class SchemaParser(object):
    def __init__(self, source=None):
        self.source = source

    def parse(self):
        tokens = self._parse_tokens(self.source)
        return self._build_schema(tokens)

    def _parse_tokens(self, string):
        LPAR, RPAR, SCOL = map(Suppress, "{};")

        atom = Word(alphanums + "_")
        expr_list = Forward()
        simple_expr = OneOrMore(atom) + SCOL
        complex_expr = OneOrMore(atom) + expr_list + Optional(SCOL)
        expr = Group(simple_expr | complex_expr).setParseAction(lambda t: t.asList())
        expr_list << LPAR + Group(ZeroOrMore(expr)) + RPAR

        return expr.parseString(string).asList()[0]

    def _build_schema(self, tokens, op=None):
        assert len(tokens) >= 1

        OP_1 = ["NOPREFIX"]
        OP_2 = ["KEY", "SEGMENTS", "JSON_NAME"]
        OP_LIST = ["SCHEMA", "TABLE", "KEYSPACE", "ROW"]
        OP_FIELD = ["FIELD"]

        NESTED_OP = {
            "ROW": "FIELD",
        }

        if op is None:
            op = tokens[0]

        if op in OP_1:
            assert len(tokens) == 1
            return {op: True}
        elif op in OP_2:
            assert len(tokens) == 2
            return {op: tokens[1]}
        elif op in OP_LIST:
            assert len(tokens) == 3
            assert isinstance(tokens[2], list)
            sctype, scname = tokens[:2]
            schema = {sctype: {scname: OrderedDict()}}
            for ts in tokens[2]:
                update = self._build_schema(ts, NESTED_OP.get(op))
                schema[sctype][scname] = self._merge_schema(schema[sctype][scname], update)
            return schema
        elif op in OP_FIELD:
            return tokens
        else:
            raise ParseException("unexpected tokens: {}".format(str(tokens)))

    def _merge_schema(self, schema, update):
        unique = ["KEY", "ROW", "JSON_NAME"]

        if isinstance(update, dict):
            for k, v in update.items():
                if k in schema and k in unique:
                    raise ParseException("duplicate field {}: {}".format(k, v))

                if isinstance(v, Mapping):
                    r = self._merge_schema(schema.get(k, OrderedDict()), v)
                    schema[k] = r
                else:
                    schema[k] = update[k]
        else:
            if not isinstance(schema, list):
                assert schema == {}
                schema = []
            schema.append(update)

        return schema
