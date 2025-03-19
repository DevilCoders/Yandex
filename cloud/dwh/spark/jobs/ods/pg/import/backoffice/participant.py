from spyt import spark_session
from spark.jobs.common import get_parser_pg_args, get_logger, get_latest_yt_table

from pyspark.sql.functions import col, to_timestamp
from yt import wrapper as yt
from spark.jobs.config import YT_RAW_LAYER_PATH, DWH_SECRET_ID, CLAN_SECRET_ID

import vault_client

import os


def main():
    parser = get_parser_pg_args()
    args = parser.parse_args()
    logger = get_logger()
    secret = os.environ["SPARK_SECRET"]

    yav = vault_client.instances.Production(authorization=secret)

    secret_dwh = yav.get_version(DWH_SECRET_ID)['value']
    pg_password = secret_dwh["dwh-etl-user-password-prod"]

    secret_clan = yav.get_version(CLAN_SECRET_ID)['value']
    yt.config["proxy"]["url"] = "hahn"
    yt.config["token"] = secret_clan["yt_token"]

    with spark_session() as spark:
        logger.info("Started.")
        folder = "{}/backoffice/participants".format(YT_RAW_LAYER_PATH)
        yt_table = get_latest_yt_table(folder)
        df = spark.read.yt(folder + "/{}".format(yt_table))

        df = df.withColumn("created_at", to_timestamp("created_at")) \
            .withColumn("updated_at", to_timestamp("updated_at"))

        mode = "overwrite"
        url = "jdbc:postgresql://{}:{}/{}".format(args.pg_host, args.pg_port, args.pg_database)
        properties = {"user": args.pg_user, "password": pg_password, "driver": "org.postgresql.Driver"}
        df.coalesce(1).write.option("truncate", "true").jdbc(url=url, table="stg.bcf_participant",
                                                             mode=mode, properties=properties)
        logger.info("Done.")


if __name__ == '__main__':
    main()
