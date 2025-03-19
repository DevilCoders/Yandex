import argparse
import logging

from yt import wrapper as yt
from pyspark.sql.functions import col, to_timestamp


# todo rewrite for builder pattern

def get_parser_pg_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pg_host", help="postgres host", required=True)
    parser.add_argument("--pg_port", help="postgres port", required=True)
    parser.add_argument("--pg_database", help="postgres database", required=True)
    parser.add_argument("--pg_user", help="postgres username", required=True)
    return parser


def get_parser_pg_args_1secret():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pg_host", help="postgres host", required=True)
    parser.add_argument("--pg_port", help="postgres port", required=True)
    parser.add_argument("--pg_database", help="postgres database", required=True)
    parser.add_argument("--pg_user", help="postgres username", required=True)
    parser.add_argument("--secret", help="postgres password", required=True)
    return parser


def get_parser_pg_args_2secrets():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pg_host", help="postgres host", required=True)
    parser.add_argument("--pg_port", help="postgres port", required=True)
    parser.add_argument("--pg_database", help="postgres database", required=True)
    parser.add_argument("--pg_user", help="postgres username", required=True)
    parser.add_argument("--secret1", help="secret 1", required=True)
    parser.add_argument("--secret2", help="secret 2", required=True)
    return parser


def get_logger():
    logging.basicConfig()
    logger = logging.getLogger()
    logger.setLevel(logging.INFO)
    return logger


def prepare_for_yt(df):
    for name, typ in df.dtypes:
        if typ == "timestamp":
            df = df.withColumn(name, col(name).cast("long"))
        elif typ == "date":
            df = df.withColumn(name, to_timestamp(col(name)).cast("long"))
        elif typ.startswith("decimal"):
            df = df.withColumn(name, col(name).cast("double"))
    return df


def get_latest_yt_table(yt_folder):
    table_list = yt.list(yt_folder, attributes=["creation_time", "type"], sort=False)
    table_list = [table for table in table_list if table.attributes["type"] == "table"]
    table_list.sort(key=lambda item: item.attributes["creation_time"])
    if len(table_list) == 0:
        return
    return str(table_list[-1])
