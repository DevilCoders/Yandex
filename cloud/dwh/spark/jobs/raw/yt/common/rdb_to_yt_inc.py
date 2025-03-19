import argparse

from spyt import spark_session
from spark.jobs.common import get_logger, prepare_for_yt
from datetime import datetime
from pyspark.sql.functions import col, to_timestamp, unix_timestamp
import pytz
import os


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", help="host", required=True)
    parser.add_argument("--port", help="port", required=True)
    parser.add_argument("--database", help="database", required=True)
    parser.add_argument("--user", help="username", required=True)
    parser.add_argument("--driver", help="driver class", required=True)
    parser.add_argument("--source_table", help="db table", required=True)
    parser.add_argument("--yt_path", help="target yt path", required=True)
    parser.add_argument("--num_partitions", help="numPartitions", required=False, type=str)
    parser.add_argument("--partition_column", help="partitionColumn", required=False)
    parser.add_argument("--lower_bound", help="lowerBound", required=False, type=str)
    parser.add_argument("--upper_bound", help="upperBound", required=False, type=str)
    parser.add_argument("--increment_column_name", help="increment column name", required=True, type=str)
    parser.add_argument("--increment_column_type", help="increment column type", required=True, type=str)
    parser.add_argument("--increment_column_value", help="increment column value", required=True, type=str)
    parser.add_argument("--overwrite", help="overwrite or create new", default=False, action='store_true')
    parser.add_argument("--append", help="append to an existing table", default=False, action='store_true')
    return parser.parse_args()


def get_url_for_jdbc(args):
    db = args.driver.split(".")[1]
    url = "jdbc:{}://{}:{}/{}?serverTimezone=UTC".format(db, args.host, args.port, args.database)
    return url


def get_properties_for_jdbc(args):
    props = {
        "user": args.user,
        "password": args.secret,
        "driver": args.driver  # org.postgresql.Driver
    }
    if args.num_partitions:
        props["numPartitions"] = args.num_partitions
    else:
        props["numPartitions"] = '1'

    if args.partition_column:
        if not args.upper_bound or not args.lower_bound:
            raise ValueError("when --partition_column is provided, both --lower_bound and --upper_bound must be "
                             "provided")
        props["partitionColumn"] = args.partition_column
        props["lowerBound"] = args.lower_bound
        props["upperBound"] = args.upper_bound
    return props


def filter_by_increment_column(df, inc_column, inc_column_type, inc_column_value):
    if inc_column_type in ("date", "datetime"):
        inc_dttm = datetime.strptime(inc_column_value, "%Y%m%d%H%M%S")
        inc_dttm = int((inc_dttm - datetime(1970, 1, 1)).total_seconds())
        return df \
            .withColumn("_lms_ts_column", unix_timestamp(col(inc_column))) \
            .filter(col("_lms_ts_column") > inc_dttm) \
            .drop("_lms_ts_column")
    elif inc_column_type == "integer":
        return df.filter(col(inc_column) > int(inc_column_value))
    raise ValueError("Invalid increment column type")


def main():
    args = parse_args()
    args.secret = os.environ["SPARK_SECRET"]
    logger = get_logger()
    props = get_properties_for_jdbc(args)
    url = get_url_for_jdbc(args)

    if args.overwrite and args.append:
        raise Exception("Both --overwrite and --append are specified")

    with spark_session() as spark:
        logger.info("Started.")
        df = spark\
            .read\
            .jdbc(url=url, table=args.source_table, properties=props)

        df = filter_by_increment_column(df, args.increment_column_name, args.increment_column_type, args.increment_column_value)
        logger.info("Filtered.")

        if df.count() == 0:
            logger.info("DataFrame is empty -- exiting.")
            return

        df = prepare_for_yt(df)
        df = df.coalesce(1)

        if args.overwrite:
            logger.info("overwriting " + args.yt_path)
            df.write.mode("overwrite").optimize_for("scan").yt(args.yt_path)
        elif args.append:
            logger.info("appending to " + args.yt_path)
            df.write.mode("append").optimize_for("scan").yt(args.yt_path)
        else:
            table_name = datetime.now(tz=pytz.utc).strftime('%Y-%m-%dT%H:%M:%S')
            logger.info("creating new table: " + args.yt_path + "/" + table_name)
            df.write.optimize_for("scan").yt(args.yt_path + "/" + table_name)
        logger.info("Done.")


if __name__ == '__main__':
    main()
