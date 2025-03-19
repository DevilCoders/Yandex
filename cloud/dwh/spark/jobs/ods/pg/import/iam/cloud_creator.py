from spyt import spark_session
from spark.jobs.common import get_parser_pg_args, get_logger
import py4j

from pyspark.sql.functions import col, to_timestamp

import os

logger = get_logger()


def main():
    parser = get_parser_pg_args()
    secret = os.environ["SPARK_SECRET"]
    args = parser.parse_args()

    with spark_session() as spark:
        logger.info("Started.")
        df_cloud_creator = spark.read.yt("//home/cloud_analytics/import/iam/cloud_owners/1h/latest")
        mode = "overwrite"
        url = "jdbc:postgresql://{}:{}/{}".format(args.pg_host, args.pg_port, args.pg_database)
        properties = {"user": args.pg_user, "password": secret, "driver": "org.postgresql.Driver"}
        logger.info("Start reading table")
        df_cloud_creator \
            .drop("_other") \
            .withColumn("cloud_created_at", to_timestamp(col("cloud_created_at"), "yyyy-MM-dd'T'HH:mm:ss+00:00")) \
            .withColumn("passport_uid", col("passport_uid").cast("long")) \
            .coalesce(1) \
            .write \
            .option("truncate", "true") \
            .jdbc(url=url, table="stg.iam_cloud_creator", mode=mode, properties=properties)
        logger.info("Done.")


if __name__ == '__main__':
    try:
        main()
    except py4j.protocol.Py4JJavaError as err:
        logger.info(str(err.__unicode__().encode('utf-8')))
        raise err
