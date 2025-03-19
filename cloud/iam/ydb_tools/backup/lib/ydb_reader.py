# coding=utf-8

import os
import urllib2

from google.protobuf import text_format

import ydb.public.api.protos.ydb_table_pb2 as ydb_table_proto
import ydb.public.api.protos.ydb_value_pb2 as ydb_value_proto

# import ydo.database.util.data_definition as util_data_definition
# import ydo.database.util.kikimr_wraps as util_kikimr_wraps

type_mapping = {id: name.lower() for name, id in ydb_value_proto.Type.PrimitiveTypeId.items()}


def type_parser(t):
    return type_mapping[t.optional_type.item.type_id]


def wrap_column(column, type_parser):
    column_type = type_parser(column.type)
    return dict(
        name=column.name,
        type=column_type,
    )


def parse_table_schema(describe_table_result, type_parser, no_sort=False):
    key_column_to_index = {key_column: i for i, key_column in enumerate(describe_table_result.primary_key)}
    table = [dict(primary_key=column.name in key_column_to_index, **wrap_column(column, type_parser))
             for column in describe_table_result.columns]
    if no_sort:
        return table
    table = sorted(table, key=lambda column: key_column_to_index.get(column["name"], len(key_column_to_index)))
    return table


def read_table_schema(stream):
    create_table_request = ydb_table_proto.CreateTableRequest()
    text_format.Parse(stream.read(), create_table_request)
    return parse_table_schema(create_table_request, type_parser), \
           parse_table_schema(create_table_request, type_parser, no_sort=True)


def cgi_unquote(value):
    return urllib2.unquote(value[1:-1].replace('+', ' '))


def parse_bool(value):
    return bool(int(value))


def nullable(func):
    def wrapped(value):
        if value == 'null':
            return None
        return func(value)

    return wrapped


to_python_type = {
    "utf8": str,
    "string": str,
    "uint64": int,
    "int64": int,
    "uint32": int,
    "int32": int,
    "float": float,
    "double": float,
    "bool": bool,
    "boolean": bool,
}


def get_parser(type_name):
    if type_name == 'string' or type_name == "utf8":
        return nullable(cgi_unquote)
    elif type_name == 'bool':
        return nullable(parse_bool)
    else:
        return nullable(to_python_type[type_name])


def get_parsers(schema):
    return [get_parser(column['type']) for column in schema]


def get_names(schema):
    return [column['name'] for column in schema]


def read_table_part(parsers, names, stream, filter_columns=None):
    assert len(parsers) == len(names)
    for line in stream:
        values = line.strip().split(',')
        assert len(values) == len(parsers)
        yield {name: parser(value) for name, parser, value in zip(names, parsers, values) if filter_columns is None or name in filter_columns}


def read_table(folder='', filter_columns=None):
    with open(os.path.join(folder, 'scheme.pb')) as scheme:
        sorted_schema, schema = read_table_schema(scheme)

    parsers, names = get_parsers(schema), get_names(schema)

    def iter():
        i = 0
        while os.path.exists(os.path.join(folder, 'data_%02d.csv' % i)):
            with open(os.path.join(folder, 'data_%02d.csv' % i)) as part:
                for row in read_table_part(parsers, names, part, filter_columns):
                    yield row
            i += 1

    return sorted_schema, iter
