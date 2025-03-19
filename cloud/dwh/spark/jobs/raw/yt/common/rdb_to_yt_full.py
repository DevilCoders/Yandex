import argparse

from spyt import spark_session
from spark.jobs.common import get_logger, prepare_for_yt
from datetime import datetime
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
    parser.add_argument("--overwrite", help="overwrite or create new", default=False, action='store_true')
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


def main():
    args = parse_args()
    args.secret = os.environ["SPARK_SECRET"]
    logger = get_logger()
    props = get_properties_for_jdbc(args)
    url = get_url_for_jdbc(args)

    with spark_session() as spark:
        logger.info("Started.")
        tables = args.source_table.split(",")
        yt_paths = args.yt_path.split(",")
        for i, table in enumerate(tables):
            logger.info("Loading {}".format(table))
            df = spark.read.jdbc(url=url, table=table, properties=props)
            df = prepare_for_yt(df)
            df = df.coalesce(1)
            if args.overwrite:
                logger.info("overwriting " + yt_paths[i])
                df.write.mode("overwrite").optimize_for("scan").yt(yt_paths[i])
            else:
                table_name = datetime.now(tz=pytz.utc).strftime('%Y-%m-%dT%H:%M:%S')
                logger.info("creating new table: " + yt_paths[i] + "/" + table_name)
                df.write.optimize_for("scan").yt(yt_paths[i] + "/" + table_name)
        logger.info("Done.")


if __name__ == '__main__':
    main()
