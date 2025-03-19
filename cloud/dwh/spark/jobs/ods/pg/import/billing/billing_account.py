from spyt import spark_session
from spark.jobs.common import get_parser_pg_args, get_logger

from pyspark.sql.functions import col, from_json, to_timestamp
from pyspark.sql.types import StringType, StructType, IntegerType, StructField, BooleanType

import os


def main():
    parser = get_parser_pg_args()
    args = parser.parse_args()
    logger = get_logger()
    secret = os.environ["SPARK_SECRET"]

    with spark_session() as spark:
        logger.info("Started.")
        df_billing = spark.read \
            .option("arrow_enabled", "false") \
            .schema_hint({
                "metadata": StringType(),
                "feature_flags": StringType()
            }) \
            .yt("//home/cloud/billing/exported-billing-tables/billing_accounts_prod")

        metadata_schema = StructType([
            StructField("paid_at", IntegerType(), True),
            StructField("block_reason", StringType(), True),
            StructField("unblock_reason", StringType(), True)
        ])

        feature_schema = StructType([
            StructField("isv", BooleanType(), True),
            StructField("var", BooleanType(), True)
        ])

        df_billing = df_billing \
            .withColumn("metadata", from_json("metadata", metadata_schema)) \
            .withColumn("feature_flags", from_json("feature_flags", feature_schema)) \
            .withColumn("export_ts", to_timestamp("export_ts")) \
            .withColumn("created_at", to_timestamp("created_at")) \
            .withColumn("updated_at", to_timestamp("updated_at")) \
            .withColumn("owner_id", col("owner_id").cast("long")) \
            .withColumnRenamed("id", "ba_id")

        df_billing = df_billing \
            .withColumn("is_isv", col("feature_flags.isv")) \
            .withColumn("is_var", col("feature_flags.var")) \
            .withColumn("paid_at", to_timestamp("metadata.paid_at")) \
            .withColumn("block_reason", col("metadata.block_reason")) \
            .withColumn("unblock_reason", col("metadata.unblock_reason")) \
            .drop("metadata", "feature_flags") \
            .select("name", "balance", "payment_method_id", "export_ts", "person_id", "currency", "state",
                    "balance_client_id", "person_type", "ba_id", "country_code", "payment_cycle_type", "owner_id",
                    "master_account_id", "usage_status", "type", "billing_threshold", "created_at", "balance_contract_id",
                    "updated_at", "client_id", "payment_type", "is_isv", "is_var", "block_reason", "unblock_reason",
                    "paid_at")

        mode = "overwrite"
        url = "jdbc:postgresql://{}:{}/{}".format(args.pg_host, args.pg_port, args.pg_database)
        properties = {"user": args.pg_user, "password": secret, "driver": "org.postgresql.Driver"}
        logger.info("Start reading table")
        df_billing.coalesce(1).write.option("truncate", "true").jdbc(url=url, table="ods.bln_billing_account",
                                                                     mode=mode, properties=properties)
        logger.info("Done.")


if __name__ == '__main__':
    main()
