import pandas as pd
from typing import Tuple
import pyspark.sql.functions as F
from pyspark.sql.window import Window
from pyspark.sql.functions import col, lit
from pyspark.sql.session import SparkSession
from pyspark.sql.dataframe import DataFrame as SparkDataFrame
from pyspark.sql.column import Column as SparkColumn

BA_ID_COL_NAME = 'billing_account_id'
BR_MSK_DT_NAME = 'billing_record_msk_date'
Y_TRUE_COL_NAME = 'next_14d_cons'
Y_PRED_COL_NAME = 'next_14d_cons_pred'


def max_by(colname_max: str, colname_by: str) -> SparkColumn:
    """max_by takes two SparkColumn names and returns the value of the first one
    for which the value of the second one is maximized (aggragation function)

    Parameters
    ----------
    colname_max: str
        value of SparkColumn with that name to be returned, when the other argument is maximized
    colname_by: Any
        value of SparkColumn with that name to be maximized

    Returns
    ------
    SparkColumn
        resulting SparkColumn with described calculation
    """
    return F.expr(f'max_by({colname_max}, {colname_by})')


def min_by(colname_min, colname_by):
    """min_by takes two SparkColumn names and returns the value of the first one
    for which the value of the second one is minimized (aggragation function)

    Parameters
    ----------
    colname_max: str
        value of SparkColumn with that name to be returned, when the other argument is minimized
    colname_by: Any
        value of SparkColumn with that name to be minimized

    Returns
    ------
    SparkColumn
        resulting SparkColumn with described calculation
    """
    return F.expr(f'min_by({colname_min}, {colname_by})')


def date_to_bigint(dt_col):
    return F.datediff(dt_col, lit("1900-01-01")).cast("bigint")


def make_window(N):
    if (N == 0):
        raise ValueError("N must be greater or less than 0")
    if N > 0:
        w = (
            Window
            .partitionBy("billing_account_id")
            .orderBy(date_to_bigint("billing_record_msk_date"))
            .rangeBetween(1, N)
        )
    else:
        w = (
            Window
            .partitionBy("billing_account_id")
            .orderBy(date_to_bigint("billing_record_msk_date"))
            .rangeBetween(N, -1)
        )
    return w


def next_N_days_cons(N, spark_col="billing_record_total_rub", pfx="cons"):
    ans = F.sum(spark_col).over(make_window(N))
    w = Window.partitionBy("billing_account_id")
    cond = (F.datediff(F.max("billing_record_msk_date").over(w), col("billing_record_msk_date"))>=N)
    colname = f"prev_{abs(N)}d_{pfx}" if (N<0) else f"next_{N}d_cons"
    return F.when(cond, ans).otherwise(lit(None)).alias(colname)


def make_pool(spark: SparkSession, table: SparkDataFrame) -> Tuple[SparkDataFrame]:
    """Builds DSV-formatted SparkDataFrame of table and its column description

    Parameters
    ----------
    spark: SparkSession
        SparkSession to use in pipeline
    table: SparkDataFrame
        SparkDataFrame table to transform in DSV-formatted SparkDataFrame

    Returns
    ------
    Tuple[SparkDataFrame]
        SparkDataFrame of transformed table with columns:
        * key - target
        * value - features

        SparkDataFrame with columns description:
        * key - column number
        * value - type and name of column
    """
    pool_columns = table.columns.copy()
    key_column = 'next_14d_cons'
    descr = ['Label']
    pool_columns.remove(key_column)

    value_formula_concat_list = []
    for colname in pool_columns:
        value_formula_concat_list.append(lit("\t"))
        if colname in ["billing_account_id", "br_week_num", "br_week_day", "billing_record_msk_date", "billing_account_usage_status"]:
            descr.append('Auxiliary')
            value_formula_concat_list.append(F.coalesce(col(colname).cast("string"), lit("NULL")))
        elif colname == "SampleId":
            descr.append('SampleId')
            value_formula_concat_list.append(col(colname))
        elif colname in ["billing_account_person_type", "billing_account_currency", "billing_account_is_fraud",
                         "billing_account_is_suspended_by_antifraud", "billing_account_is_isv", "billing_account_is_var",
                         "billing_account_is_crm_account", "crm_partner_manager", "crm_segment", "sku_lazy", "billing_account_state"]:
            descr.append(f'Categ\t{colname}')
            value_formula_concat_list.append(F.coalesce(col(colname).cast("string"), lit("NULL")))
        else:
            descr.append(f'Num\t{colname}')
            value_formula_concat_list.append(F.coalesce(col(colname), lit(0)))
    value_formula_concat_list = value_formula_concat_list[1:]
    result = table.select(col(key_column).cast("string").alias("key"), F.concat(*value_formula_concat_list).alias("value"))
    cd = pd.Series(descr).reset_index()
    cd.columns = ['key', 'value']
    cd = (
        spark.createDataFrame(cd)
        .sort("key")
        .select(
            col("key").cast("string").alias("key"),
            col("value").cast("string").alias("value")
        )
        .coalesce(1)
    )
    return result, cd


def read_preprocess_catboost_result_spark(spark: SparkSession, path: str) -> SparkDataFrame:
    spdf = (
        spark.read.yt(path)
        .select(
            F.split(col('SampleID'), '_')[0].alias(BA_ID_COL_NAME),
            (F.split(col('SampleID'), '_')[1]).alias(BR_MSK_DT_NAME),
            col('Label').astype('double').alias(Y_TRUE_COL_NAME),
            col('RawFormulaVal').astype('double').alias(Y_PRED_COL_NAME),
        )
    )
    return spdf
