import os
import sys
import logging

import yt.wrapper

FORMAT = "%(asctime)s:%(levelname)s:%(name)s:%(message)s"
logging.basicConfig(level=logging.DEBUG, format=FORMAT)

sys.path.append(os.path.abspath(os.path.dirname(__file__)))
from core.query import run_query, run_query_stream, run_query_format_json


def create_new_yt_client():
    return yt.wrapper.YtClient(proxy="banach.yt.yandex.net")


def convert_clickhouse_schema_to_yt_schema(clickhouse_schema):
    type_conversion = {
        'UInt8': 'uint64',
        'UInt16': 'uint64',
        'UInt32': 'uint64',
        'UInt64': 'uint64',

        'Int8': 'int64',
        'Int16': 'int64',
        'Int32': 'int64',
        'Int64': 'int64',

        'Float32': 'double',
        'Float64': 'double',

        'Date': 'string',
        'String': 'string'
    }

    yt_schema = [{
        'name': column_desc['name'],
        'type': type_conversion[column_desc['type']]
        } for column_desc in clickhouse_schema
    ]

    return yt_schema


def coerce_ch_row(d, name_to_type):
    """ For some reason clickhouse doesn't coerce types properly.
        This function converts int columns to ints and floats to float """

    res = {}
    for column, value in d.iteritems():
        if name_to_type[column] == "double":
            res[column] = float(value)
        else:
            res[column] = value

    return res


def squeeze_table(yt_client, yt_table):
    yt_client.transform(yt_table,
                        yt_table,
                        erasure_codec="lrc_12_2_2",
                        compression_codec="brotli_6",
                        check_codecs=True)


def upload_to_yt(clickhouse_table, date, yt_table, clickhouse_host=None, yt_client=None):
    if yt_client is None:
        yt_client = create_new_yt_client()

    rows_count_query = """
        SELECT count() FROM {table}
        WHERE eventDate = '{date}'
    """.format(
        table=clickhouse_table,
        date=date
    )
    clickhouse_rows_count = int(run_query(rows_count_query, hosts=clickhouse_host)[0][0])

    ts_range_query = """
        SELECT min(ts), max(ts) FROM {table}
        WHERE eventDate = '{date}'
    """.format(
        table=clickhouse_table,
        date=date
    )
    min_ts, max_ts = map(int, run_query(ts_range_query, hosts=clickhouse_host)[0])
    assert max_ts - min_ts <= 24 * 60 * 60

    ch_schema = run_query_format_json("DESCRIBE TABLE {table} FORMAT JSON".format(table=clickhouse_table))['data']
    yt_schema = convert_clickhouse_schema_to_yt_schema(ch_schema)

    # TODO: check rows_count
    assert not yt_client.exists(yt_table) or yt_client.row_count(yt_table) == 0
    if not yt_client.exists(yt_table):
        yt_client.create_table(yt_table, attributes={"schema": yt_schema, 'dynamic': False}, recursive=True)

    yt_client.set(yt_table + "/@erasure_codec", "lrc_12_2_2")
    yt_client.set(yt_table + "/@compression_codec", "brotli_6")

    # start_ts/end_ts were originally intended for writing in small chunks.
    # With the current code there is no need for them
    data_query = """
        SELECT * FROM {table}
        WHERE eventDate = '{date}' AND ts>={start_ts} AND ts <= {end_ts}
        FORMAT JSONEachRow
    """.format(
        table=clickhouse_table,
        date=date,
        start_ts=min_ts,
        end_ts=max_ts
    )

    r = run_query_stream(data_query, hosts=clickhouse_host)

    with yt_client.Transaction():
        yt_client.write_table(
            yt_client.TablePath(name=yt_table, append=True),
            r.iter_lines(),
            format=yt.wrapper.JsonFormat(attributes={"enable_integral_to_double_conversion": True, "plain": True}),
            raw=True
        )
        uploaded_row_count = yt_client.row_count(yt_table)
        if uploaded_row_count != clickhouse_rows_count:
            raise Exception(
                "Uploaded {} rows, but expected to upload {} rows".format(uploaded_row_count, clickhouse_rows_count)
            )

        logging.info("Successfully uploaded %s", yt_table)


def check_and_upload_missing_days(clickhouse_table, clickhouse_host, start_date, end_date):
    clickhouse_dates_query = """SELECT DISTINCT eventDate from {table}
                                WHERE eventDate >= '{start_date}' AND eventDate <= '{end_date}'
                                ORDER BY eventDate
                            """.format(table=clickhouse_table, start_date=start_date, end_date=end_date)
    clickhouse_dates = [row[0] for row in run_query(clickhouse_dates_query, hosts=clickhouse_host)]

    yt_client = create_new_yt_client()
    yt_dates = yt_client.list("//home/rxclickhouse/{}".format(clickhouse_table))

    dates_to_upload = sorted(set(clickhouse_dates) - set(yt_dates))

    for date in dates_to_upload:
        yt_table = "//home/rxclickhouse/{}/{}".format(clickhouse_table, date)
        upload_to_yt(clickhouse_table, date, yt_table, clickhouse_host=clickhouse_host)


def main(clickhouse_table, clickhouse_host, start_date, end_date):
    check_and_upload_missing_days(clickhouse_table, clickhouse_host, start_date, end_date)


if __name__ == "__main__":
    assert len(sys.argv) == 5
    main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
