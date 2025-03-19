import pandas as pd
from typing import List, Dict
import pyspark.sql.types as T
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from consumption_predictor_v2.feature_collection.config_mf_skus import config_mf_skus
from pyspark.sql.session import SparkSession
from pyspark.sql.dataframe import DataFrame as SparkDataFrame
from pyspark.sql.column import Column as SparkColumn


def make_grid(spark: SparkSession, period_start: str, period_end: str) -> SparkDataFrame:
    """Builds SparkDataFrame with grid of all billing accounts over all dates later then date of creation

    Parameters
    ----------
    spark: SparkSession
        SparkSession to use in pipeline
    period_start: str
        string with date (ISO-format) to begin grid from
    period_end: str
        string with date (ISO-format) to end grid in

    Returns
    ------
    SparkDataFrame
        SparkDataFrame with columns:
        * billing_account_id
        * billing_record_msk_date
        * days_from_created
    """
    ba_info = "//home/cloud-dwh/data/prod/ods/billing/billing_accounts"
    date_grid = (
        spark
        .createDataFrame(pd.DataFrame({"event_date": pd.date_range(period_start, period_end)}))
        .select(col("event_date").cast("date"))
    )

    all_ba_grid = (
        spark.read.yt(ba_info)
        .filter(col("usage_status")!="service")
        .filter(~col("is_suspended_by_antifraud"))
        .select(
            "billing_account_id",
            F.to_date(F.to_timestamp(col("created_at").cast("double") * 1000000)).alias("start_date"), # temporary solution on spyt bug (see https://st.yandex-team.ru/SPYT-332)
        )
        .distinct()
        .join(date_grid, on=(col("start_date")<=col("event_date")), how="cross")
        .select(
            "billing_account_id",
            F.date_format(col("event_date"), "yyyy-MM-dd").alias("billing_record_msk_date"),
            F.datediff(col("event_date"), col("start_date")).alias("days_from_created")
        )
        .cache()
    )
    return all_ba_grid


def sku_related_features(config_mf_skus: Dict[str, List[str]] = config_mf_skus) -> List[SparkColumn]:
    """Describes SparkColumns building. WARN: Used only for make_yc_cons function

    Each SparkColumn defines Sum of cunsumption cost over specialised SKU-group, -service or subservice.

    Parameters
    ----------
    config_mf_skus: Dict[str, List[str]]
        Special config with 3 keys ("names_all_sku_sgps", "names_all_sku_srvs", "names_mf_sku_sbsrvs").
        Value of each key contains list of groups (for "names_all_sku_sgps"), services (for "names_all_sku_srvs")
        and subservices (for "names_mf_sku_sbsrvs") that must be accounted separately for billing-account at given date

    Returns
    ------
    List[SparkColumn]
        List of predefined columns according to given config
    """
    sku_group_cols = []
    for name in config_mf_skus["names_all_sku_sgps"]:
        col_name = name.replace(" ", "_").replace(".", "_").replace("-", "_")
        sku_group_cols.append(
            F.coalesce(F.sum(
                F.when(col("sku_service_group")==name, col("billing_record_total_rub"))
                .otherwise(lit(0.0))
            ), lit(0.0)).alias(f"sku_group_is_{col_name}")
        )

    sku_service_cols = []
    for name in config_mf_skus["names_all_sku_srvs"]:
        col_name = name.replace(" ", "_").replace(".", "_").replace("-", "_")
        sku_service_cols.append(
            F.coalesce(F.sum(
                F.when(col("sku_service_name")==name, col("billing_record_total_rub"))
                .otherwise(lit(0.0))
            ), lit(0.0)).alias(f"sku_service_is_{col_name}")
        )

    sku_subservice_cols = []
    for name in config_mf_skus["names_mf_sku_sbsrvs"]:
        col_name = name.replace(" ", "_").replace(".", "_").replace("-", "_")
        sku_subservice_cols.append(
            F.coalesce(F.sum(
                F.when(col("sku_subservice_name")==name, col("billing_record_total_rub"))
                .otherwise(lit(0))
            ), lit(0.0)).alias(f"sku_subservice_is_{col_name}")
        )

    sku_subservice_cols.append(
        F.coalesce(F.sum(
            F.when(~col("sku_subservice_name").isin(config_mf_skus["names_mf_sku_sbsrvs"]), col("billing_record_total_rub"))
            .otherwise(lit(0.0))
        ), lit(0.0)).alias("sku_subservice_name_is_other")
    )

    result_columns = sku_group_cols + sku_service_cols + sku_subservice_cols
    return result_columns


def make_yc_cons(spark: SparkSession,
                 all_ba_grid: SparkDataFrame) -> SparkDataFrame:
    """Builds SparkDataFrame with consumption features based on given grid of billing-accounts and dates

    Parameters
    ----------
    spark: SparkSession
        SparkSession to use in pipeline
    all_ba_grid: SparkDataFrame
        SparkDataFrame with grid of billing-accounts and dates (column "days_from_created" is also need)

    Returns
    ------
    SparkDataFrame
        SparkDataFrame with consumption features. Has columns:
        * billing_account_id
        * billing_record_msk_date
        * billing_record_cost_rub
        * billing_record_credit_rub
        * billing_record_total_rub
        * [ group of columns defined in function "sku_related_features" ]
        * days_from_created
        * billing_account_usage_status
        * billing_account_person_type
        * billing_account_currency
        * billing_account_state
        * billing_account_is_fraud
        * billing_account_is_suspended_by_antifraud
        * billing_account_is_isv
        * billing_account_is_var
        * billing_account_is_crm_account
        * crm_partner_manager
        * crm_segment
    """
    dm_yc_cons_path = "//home/cloud-dwh/data/prod/cdm/dm_yc_consumption"
    dm_crm_tags_path = "//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags"
    additional_cols = sku_related_features()

    spdf_crm_tags = (
        all_ba_grid
        .join(
            spark.read.yt(dm_crm_tags_path).withColumn("billing_record_msk_date", col("date")),
            on=["billing_account_id", "billing_record_msk_date"], how="left"
        )
        .select(
            "billing_account_id",
            "billing_record_msk_date",
            F.least(F.coalesce("days_from_created", lit(0)), lit(365)).alias("days_from_created"),
            col("usage_status").alias("billing_account_usage_status"),
            col("person_type").alias("billing_account_person_type"),
            col("currency").alias("billing_account_currency"),
            col("state").alias("billing_account_state"),
            col("is_fraud").alias("billing_account_is_fraud"),
            col("is_suspended_by_antifraud").alias("billing_account_is_suspended_by_antifraud"),
            col("is_isv").alias("billing_account_is_isv"),
            col("is_var").alias("billing_account_is_var"),
            col("crm_account_id").isNotNull().alias("billing_account_is_crm_account"),
            col("partner_manager").alias("crm_partner_manager"),
            col("segment").alias("crm_segment"),
        )
    )

    spdf_yc_cons = (
        all_ba_grid
        .join(spark.read.yt(dm_yc_cons_path), on=["billing_account_id", "billing_record_msk_date"], how="left")
        .select(
            "billing_account_id",
            col("billing_record_msk_date"),
            col("billing_record_cost_rub"),
            col("billing_record_credit_rub"),
            col("billing_record_total_rub"),
            col("sku_lazy"),
            col("sku_name"),
            col("sku_service_group"),
            col("sku_service_name"),
            col("sku_subservice_name")
        )
        .groupby("billing_account_id", "billing_record_msk_date")
        .agg(
            F.coalesce(F.sum("billing_record_cost_rub"), lit(0.0)).alias("billing_record_cost_rub"),
            F.coalesce(F.sum("billing_record_credit_rub"), lit(0.0)).alias("billing_record_credit_rub"),
            F.coalesce(F.sum("billing_record_total_rub"), lit(0.0)).alias("billing_record_total_rub"),
            *additional_cols
        )
        .join(spdf_crm_tags, on=["billing_account_id", "billing_record_msk_date"], how="left")
    )

    return spdf_yc_cons


def make_vm_cube(spark: SparkSession,
                 all_ba_grid: SparkDataFrame) -> SparkDataFrame:
    """Builds SparkDataFrame with features about virtual machines based on given grid of billing-accounts and dates

    Parameters
    ----------
    spark: SparkSession
        SparkSession to use in pipeline
    all_ba_grid: SparkDataFrame
        SparkDataFrame with grid of billing-accounts and dates (column "days_from_created" is not necessary)

    Returns
    ------
    SparkDataFrame
        SparkDataFrame with consumption features. Has columns:
        * billing_account_id
        * billing_record_msk_date
        * [ sum, avg, min, max about cores on VMs ]
        * [ sum, avg, min, max about GPUs on VMs ]
        * [ sum, avg, min, max about RAM-configurations on VMs ]
        * [ sum, avg, min, max about age of running VMs as of "billing_record_msk_date" ]
        * count of running VMs as of "billing_record_msk_date"
    """
    dm_vm_cube_path = "//home/cloud_analytics/compute_logs/vm_cube/vm_cube"

    spdf_vm_cube_base = (
        spark.read.yt(dm_vm_cube_path)
        .groupby(
            col("ba_id").alias("billing_account_id"),
            F.to_date(F.from_unixtime(col("slice_time").cast(T.LongType()))).alias("billing_record_msk_date"),
            "vm_id"
        )
        .agg(
            F.max("vm_cores").cast(T.DoubleType()).alias("cores"),
            F.max("vm_gpus").cast(T.LongType()).alias("gpus"),
            F.max("vm_memory").cast(T.DoubleType()).alias("memory"),
            F.max("vm_age_days").cast(T.DoubleType()).alias("vm_age_days")
        )
        .cache()
    )

    spdf_vm_cube = (
        all_ba_grid
        .join(spdf_vm_cube_base, on=["billing_account_id", "billing_record_msk_date"], how="left")
        .groupby("billing_account_id", "billing_record_msk_date")
        .agg(
            F.coalesce(F.sum("cores"), lit(0)).alias("vm_cores_sum"),
            F.coalesce(F.mean("cores"), lit(0)).alias("vm_cores_avg"),
            F.coalesce(F.min("cores"), lit(0)).alias("vm_cores_min"),
            F.coalesce(F.max("cores"), lit(0)).alias("vm_cores_max"),
            F.coalesce(F.sum("gpus"), lit(0)).alias("vm_gpus_sum"),
            F.coalesce(F.mean("gpus"), lit(0)).alias("vm_gpus_avg"),
            F.coalesce(F.min("gpus"), lit(0)).alias("vm_gpus_min"),
            F.coalesce(F.max("gpus"), lit(0)).alias("vm_gpus_max"),
            F.coalesce(F.sum("memory"), lit(0)).alias("vm_memory_sum"),
            F.coalesce(F.mean("memory"), lit(0)).alias("vm_memory_avg"),
            F.coalesce(F.min("memory"), lit(0)).alias("vm_memory_min"),
            F.coalesce(F.max("memory"), lit(0)).alias("vm_memory_max"),
            F.coalesce(F.sum("vm_age_days"), lit(0)).alias("vm_age_days_sum"),
            F.coalesce(F.mean("vm_age_days"), lit(0)).alias("vm_age_days_avg"),
            F.coalesce(F.min("vm_age_days"), lit(0)).alias("vm_age_days_min"),
            F.coalesce(F.max("vm_age_days"), lit(0)).alias("vm_age_days_max"),
            F.coalesce(F.count("vm_id"), lit(0)).alias("vm_count")
        )
    )

    return spdf_vm_cube
