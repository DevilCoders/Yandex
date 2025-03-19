import logging
from pprint import pprint

import click
import pandas as pd
from yt import wrapper as yt

from cloud.analytics.scripts.import_forms_events_xls.schema import schema


def rename_columns(df):
    df.columns = [field['name'] for field in schema]


def normalize_values(df):
    for field in schema:
        if 'normalize_func' in field.keys():
            column_name = field['name']
            df[column_name] = field['normalize_func'](df[column_name])


def validate_values(df):
    for field in schema:
        if 'whitelist' in field.keys():
            column_name = field['name']
            column_whitelist = field['whitelist']
            values = set(df[column_name].unique())
            invalid_values = values - column_whitelist
            if len(invalid_values) > 0:
                raise AssertionError(f"Invalid values for column {column_name}: {invalid_values}. "
                                     f"Allowed values: {column_whitelist}")


def convert_timestamp_to_datetime(events_dict):
    for e in events_dict:
        for k, v in e.items():
            if type(v) == pd.Timestamp:
                e[k] = v.to_pydatetime()


def write_table(yt_table, events_dict):
    yt_schema = [{"name": f["name"], "type": f["type"]} for f in schema]
    yt.create("table", path=yt_table, attributes={"schema": yt_schema}, force=True)
    yt.write_table(
        table=yt_table,
        input_stream=events_dict,
        raw=False
    )


@click.command()
@click.argument('xls_path')
@click.argument('yt_table')
@click.option('--proxy', help='YT Proxy', default='hahn')
def main(xls_path, yt_table, proxy):
    yt.config["proxy"]["url"] = proxy
    logging.info("parsing excel...")
    df = pd.read_excel(xls_path)
    df.reset_index(drop=True, inplace=True)
    rename_columns(df)
    normalize_values(df)
    validate_values(df)
    events_dict = df.to_dict(orient='records')
    logging.info("creating new YT table...")
    write_table(yt_table, events_dict)
    logging.info("Done.")


if __name__ == '__main__':
    main()



