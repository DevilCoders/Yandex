#!/usr/bin/env python3

import argparse
import logging

from functools import total_ordering

import pytlib.yt_io_helpers as yt_io
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc

import yt.wrapper as yt

import mstand_utils.args_helpers as mstand_uargs

from yaqlibenums import YtOperationTypeEnum


def parse_args():
    parser = argparse.ArgumentParser(description="Compare two yt tables")
    uargs.add_verbosity(parser)
    mstand_uargs.add_yt(parser)
    parser.add_argument(
        "--table1",
        help="YT path to table 1",
        required=True,
    )
    parser.add_argument(
        "--table2",
        help="YT path to table 2",
        required=True,
    )
    parser.add_argument(
        "--error-log",
        help="YT path to save differences",
    )
    parser.add_argument(
        "--key-columns",
        nargs="*",
        help="Key columns list",
    )
    parser.add_argument(
        "--skip-columns",
        nargs="*",
        help="Skip columns list",
        default=[],
    )
    uargs.add_boolean_argument(
        parser,
        "--skip-row-count-checking",
        "Skip row count checking",
    )

    return parser.parse_args()


def set_config(yt_pool=None):
    yt.config['proxy']['url'] = 'hahn.yt.yandex.net'
    yt.config['pool'] = yt_pool


JOB_SPEC = yt_io.get_yt_operation_spec(use_scheduling_tag_filter=True,
                                       operation_executor_types=YtOperationTypeEnum.MAP_REDUCE,
                                       use_porto_layer=True)


@total_ordering
class MinType(object):
    def __le__(self, other):
        return True

    def __eq__(self, other):
        return self is other


NONE = MinType()


def get_table_schema(path, skip_columns):
    schema = yt.get_attribute(path, "schema")
    return {s["name"]: s["type"] for s in schema if s["name"] not in skip_columns}


def is_sortable(data):
    return len(set(type(v) for v in data if v is not None)) <= 1


def convert_data(data):
    if isinstance(data, dict):
        return sorted([(k, convert_data(v)) for k, v in data.items()], key=lambda x: x[0])

    if isinstance(data, list):
        res = [convert_data(v) for v in data]
        if is_sortable(data):
            res.sort()
        return res
    if isinstance(data, yt.yson.yson_types.YsonStringProxy):
        return str(yt.yson.get_bytes(data))
    return NONE if data is None else data


class RowsComparator:
    def __init__(self, skip_columns):
        self.skip_columns = {"@table_index", "action_index"}
        self.skip_columns.update(skip_columns)

    def __call__(self, keys, rows):
        data = [[], []]
        for row in rows:
            data[row["@table_index"]].append({k: v for k, v in row.items() if k not in self.skip_columns})

        data_1 = convert_data(data[0])
        data_2 = convert_data(data[1])

        if len(data[0]) != len(data[1]):
            yield {
                "status": "incorrect rows count in group: {}, {}".format(len(data_1), len(data_2)),
                "keys": keys,
                "@table_index": 0,
                "column_name": None,
            }
        else:
            for row_1, row_2 in zip(data_1, data_2):
                for (name, value_1), (_, value_2) in zip(row_1, row_2):
                    if value_1 != value_2:
                        yield {
                            "status": "the columns content are different: {}, {}".format(value_1, value_2),
                            "keys": keys,
                            "@table_index": 1,
                            "column_name": name,
                        }


def copy_error_data(row):
    yield row


def get_schemas_diff(schema_1, schema_2) -> str:
    keys_1, keys_2 = set(schema_1), set(schema_2)
    sch_1_only = sorted(keys_1.difference(keys_2))
    sch_2_only = sorted(keys_2.difference(keys_1))
    result = []
    if sch_1_only:
        result.append("\tschema_1 only: {}".format(sch_1_only))
    if sch_2_only:
        result.append("\tschema_2 only: {}".format(sch_2_only))
    return "\n".join(result)


def compare_tables(path_1, path_2, error_log, key_columns, skip_columns, skip_row_count_checking):
    schema_1 = get_table_schema(path_1, skip_columns)
    schema_2 = get_table_schema(path_2, skip_columns)
    assert schema_1 == schema_2, "Tables have different schemas:\n{}".format(get_schemas_diff(schema_1, schema_2))

    row_count_1 = yt.row_count(path_1)
    row_count_2 = yt.row_count(path_2)
    if skip_row_count_checking and row_count_1 != row_count_2:
        logging.warning("Tables have different rows count: %d vs %d", row_count_1, row_count_2)
    else:
        assert row_count_1 == row_count_2, "Tables have different rows count: {} vs {}".format(row_count_1, row_count_2)

    sorted_1 = yt.get_attribute(path_1, 'sorted')
    sorted_2 = yt.get_attribute(path_2, 'sorted')
    assert sorted_1 and sorted_2, "Tables have to be sorted"

    key_columns_1 = yt.get_attribute(path_1, 'key_columns')
    key_columns_2 = yt.get_attribute(path_2, 'key_columns')
    assert key_columns_1 == key_columns_2, "Tables have different key columns"

    if key_columns is None:
        key_columns = key_columns_1
    logging.info("Key columns: %r", key_columns)

    source_tables = [
        yt.TablePath(path_1, columns=list(schema_1.keys())),
        yt.TablePath(path_2, columns=list(schema_2.keys())),
    ]

    with yt.Transaction():
        tmp_table_keys_errors = yt.create_temp_table("//tmp", prefix="compare_tables")
        tmp_table_data_errors = yt.create_temp_table("//tmp", prefix="compare_tables")

        yt.run_reduce(
            RowsComparator(skip_columns),
            source_table=source_tables,
            destination_table=[tmp_table_keys_errors, tmp_table_data_errors],
            reduce_by=key_columns,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
            spec=JOB_SPEC,
        )
        key_error_count = yt.row_count(tmp_table_keys_errors)
        data_error_count = yt.row_count(tmp_table_data_errors)
        logging.info("Total row count: %d, keys error count: %d, data error count: %d",
                     row_count_1, key_error_count, data_error_count)

        if error_log is not None and (key_error_count > 0 or data_error_count > 0):
            logging.info("Saving error log to %s", error_log)

            yt.run_map(
                copy_error_data,
                source_table=[tmp_table_keys_errors, tmp_table_data_errors],
                destination_table=error_log,
                spec=JOB_SPEC,
            )

    assert key_error_count == 0, "Tables can't compare by keys: {}".format(key_columns)
    assert data_error_count == 0, "Tables contain different data"


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)
    set_config(yt_pool=cli_args.yt_pool)

    compare_tables(
        path_1=cli_args.table1,
        path_2=cli_args.table2,
        error_log=cli_args.error_log,
        key_columns=cli_args.key_columns,
        skip_columns=cli_args.skip_columns,
        skip_row_count_checking=cli_args.skip_row_count_checking,
    )
    logging.info("Comparison completed successfully")


if __name__ == "__main__":
    main()
