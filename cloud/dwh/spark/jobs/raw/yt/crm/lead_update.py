import argparse

from spyt import spark_session
from spark.jobs.common import get_logger
from pyspark.sql.functions import col, current_timestamp, unix_timestamp


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source_yt_path", help="source yt path", required=True, type=str)
    parser.add_argument("--target_yt_path", help="target yt path", required=True, type=str)
    return parser.parse_args()


CRM_INCREMENT_COLUMN = "Timestamp"
CRM_COLUMNS = ("Timestamp", "CRM_Lead_ID", "Billing_account_id", "Status", "Description", "Assigned_to",
               "First_name", "Last_name", "Phone_1", "Phone_2", "Email", "Lead_Source", "Lead_Source_Description",
               "Callback_date", "Last_communication_date", "Promocode", "Promocode_sum", "Notes", "Dimensions",
               "Tags", "Timezone", "Account_name")


def main():
    args = parse_args()
    logger = get_logger()

    with spark_session() as spark:
        logger.info("Started.")
        df = spark\
            .read\
            .yt(args.source_yt_path)

        if CRM_INCREMENT_COLUMN in df.columns:
            raise ValueError("Column `{}` must not be present in the source dataset".format(CRM_INCREMENT_COLUMN))

        df = df.withColumn("Timestamp", unix_timestamp(current_timestamp()))

        columns_to_drop = [column for column in df.columns if column not in CRM_COLUMNS]

        df.drop(*columns_to_drop)\
            .coalesce(1) \
            .write.mode("append").optimize_for("scan").yt(args.target_yt_path)

        logger.info("Done.")


if __name__ == '__main__':
    main()
