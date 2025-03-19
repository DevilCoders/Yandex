import pandas as pd
import logging.config
from datetime import timedelta
import os
import spyt
from spyt import spark_session
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.utils.spark import DEFAULT_SPARK_CONF

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def load_data_nbs(spark: spark_session,
                  nbs_prod_used_path: str,
                  nbs_prod_purch_path: str,
                  nbs_preprod_used_path: str,
                  nbs_preprod_purch_path: str,
                  KB_in_B: int = 1024):

    logger.info('Starting to load DataFrame')
    df_nbs_prod_used = spark.read.yt(nbs_prod_used_path).select(["*", lit("prod").alias("env")]).rdd.toDF()
    df_nbs_prod_purch = spark.read.yt(nbs_prod_purch_path).select(["*", lit("prod").alias("env")]).rdd.toDF()
    df_nbs_preprod_used = spark.read.yt(nbs_preprod_used_path).select(["*", lit("preprod").alias("env")]).rdd.toDF()
    df_nbs_preprod_purch = spark.read.yt(nbs_preprod_purch_path).select(["*", lit("preprod").alias("env")]).rdd.toDF()
    df_nbs = df_nbs_prod_used.union(df_nbs_prod_purch).union(df_nbs_preprod_used).union(df_nbs_preprod_purch)

    df_nbs = (
        df_nbs
        .withColumn("time", F.to_timestamp(col("created_timestamp")))
        .withColumn("date", F.to_date(col("time")))
        .select(["date", "time", "env", col("cluster").alias("datacenter"),
                 col("media").alias("disk_type"), "value",
                 F.when(col("sensor")=="BytesCount", "used_TB").otherwise("purchased_TB").alias("metric")])
        .filter(col("datacenter").isin(["vla", "sas", "myt"]))
        .filter(col("disk_type").isin(["hdd", "ssd"]))
        .groupby(["env", "disk_type", "datacenter", "metric", "date"])
        .agg((F.max("value")/KB_in_B**4).alias("value"))
        .rdd.toDF()
    )

    df_all = (
        df_nbs
        .groupBy(["env", "disk_type", "metric", "date"])
        .agg(F.sum("value").alias("value"))
        .select(["env", "disk_type", lit("all").alias("datacenter"), "metric", "date", "value"])
        .rdd.toDF()
    )

    df_nbs = df_nbs.union(df_all).cache()
    return df_nbs


def generate_default_params():
    logger.info('Default training params generating')
    default_sarimax_params = {'p' : 1, 'd' : 1, 'q' : 1, 'P' : 0, 'D' : 0, 'Q' : 0, 'S' : 0, 'trend' : 't'}
    disk_types = ['ssd', 'ssd_nrd', 'hdd']
    datacenters = ['all', 'myt', 'vla', 'sas']
    default_params = {}
    for env in ['prod', 'preprod']:
        default_params[env] = {}
        for dtp in disk_types:
            default_params[env][dtp] = {}
            for dtc in datacenters:
                default_params[env][dtp][dtc] = {}
                for mtc in ['used_TB', 'purchased_TB']:
                    default_params[env][dtp][dtc][mtc] = {}
                    default_params[env][dtp][dtc][mtc]['sarimax'] = default_sarimax_params
                    default_params[env][dtp][dtc][mtc]['from_date'] = '2015-01-01'
                    default_params[env][dtp][dtc][mtc]['days_to_forecast'] = 200
                    default_params[env][dtp][dtc][mtc]['days_in_history'] = 180
    return default_params


def make_forecast_nbs(nbs_prod_used_path: str,
                      nbs_prod_purch_path: str,
                      nbs_preprod_used_path: str,
                      nbs_preprod_purch_path: str,
                      result_table_path: str,
                      predict_from: str = None,
                      predict_to: str = None,
                      params: dict = None,
                      spark_conf: dict = DEFAULT_SPARK_CONF,
                      chunk: int = 200*1024**2):

    yt_token = os.environ["SPARK_SECRET"] if "SPARK_SECRET" in os.environ else os.environ["YT_TOKEN"]
    yta = YTAdapter(yt_token).yt

    # spark session
    with spark_session(yt_proxy="hahn", spark_conf_args=spark_conf) as spark:
        spyt.info(spark)
        df = load_data_nbs(spark, nbs_prod_used_path, nbs_prod_purch_path,
                           nbs_preprod_used_path, nbs_preprod_purch_path)

        if (predict_from is None):
            result_table_name = result_table_path.split('/')[-1]
            result_table_folder = '/'.join(result_table_path.split('/')[:-1])
            if result_table_name in yta.list(result_table_folder):
                predict_from_tmp = spark.read.yt(result_table_path)[["training_date"]].agg(F.max("training_date")).collect()[0][0]
                predict_from = (pd.to_datetime(predict_from_tmp)+timedelta(days=1)).strftime('%Y-%m-%d')
            else:
                predict_from_tmp = '2021-05-31'
                predict_from = '2021-06-01'
        if (predict_to is None):
            predict_to = (df.select("date").agg(F.max("date")).collect()[0][0]).strftime('%Y-%m-%d')
        if predict_from > predict_to:
            logger.info('No new dates: trained: %s, available: %s' % (predict_from_tmp, predict_to))
            return None
        params = generate_default_params() if (params is None) else params
        dates = pd.date_range(start=predict_from, end=predict_to, freq='D').astype(str)

        # function for forecast calculating in aggreagation
        def forecast_metric(ts_df, params=params, date_col="date"):
            from datetime import timedelta
            import statsmodels.api as sm
            from numpy import nan

            ts_df = ts_df.sort_values(by=date_col)
            ind = pd.to_datetime(ts_df[date_col])
            ts_df = ts_df.set_index(ind)
            ts_df = ts_df.resample('D').max().ffill()
            ts_df[date_col] = ts_df.index.astype(str)

            env = ts_df['env'].values[-1]
            dtp = ts_df['disk_type'].values[-1]
            dtc = ts_df['datacenter'].values[-1]
            mtc = ts_df['metric'].values[-1]
            future = ind[ind.last_valid_index()] + timedelta(days=params[env][dtp][dtc][mtc]['days_to_forecast'])

            try:
                from_date = pd.to_datetime(params[env][dtp][dtc][mtc]['from_date'])
                ts = ts_df['value']
                ts = ts[ts.index>from_date]
                ts = ts[-params[env][dtp][dtc][mtc]['days_in_history']:]
                fit_params = params[env][dtp][dtc][mtc]['sarimax']
                model = sm.tsa.SARIMAX(ts,
                                       order=(fit_params['p'], fit_params['d'], fit_params['q']),
                                       seasonal_order=(fit_params['P'], fit_params['D'],
                                                       fit_params['Q'], fit_params['S']),
                                       trend=fit_params['trend'])
                res = model.fit(maxiter=500)
                result_df = res.get_forecast(future).summary_frame()
                result_df = result_df.clip(lower=0)
                result_df = pd.concat([ts_df, result_df], axis=1)
                result_df[date_col] = result_df.index.astype(str)
                cols_to_ffill = ['env', 'disk_type', 'datacenter', 'metric']
                result_df[cols_to_ffill] = result_df[cols_to_ffill].ffill()

            except Exception as exc:
                logger.info(exc)
                result_df = ts_df.copy()
                result_df['mean'] = nan
                result_df['mean_se'] = nan
                result_df['mean_ci_lower'] = nan
                result_df['mean_ci_upper'] = nan
            result_df = result_df.reset_index(drop=True)
            return result_df

        # calculation
        schema_list = [
            'env string',
            'disk_type string',
            'datacenter string',
            'metric string',
            'date string',
            'value double',
            'mean double',
            'mean_se double',
            'mean_ci_lower double',
            'mean_ci_upper double'
        ]

        for i, date in enumerate(dates):
            logger.info('Iter. %3d: Prediction for date %s' % (i+1, date))
            dff = (
                df.filter(col('date')<=date).groupby(["env", "disk_type", "datacenter", "metric"])
                .applyInPandas(lambda x: forecast_metric(ts_df=x), schema=', '.join(schema_list))
                .withColumn('training_date', lit(date))
            )
            dff.write.optimize_for("scan").yt(result_table_path, mode='append')
    logger.info('Result table optimization')
    yta.transform(result_table_path, desired_chunk_size=chunk, optimize_for="scan")


if __name__ == '__main__':
    nbs_prod_used_path = "//home/cloud_analytics/dwh/ods/nbs/nbs_disk_used_space"
    nbs_prod_purch_path = "//home/cloud_analytics/dwh/ods/nbs/nbs_disk_purchased_space"
    nbs_preprod_used_path = "//home/cloud_analytics/dwh_preprod/ods/nbs/nbs_disk_used_space"
    nbs_preprod_purch_path = "//home/cloud_analytics/dwh_preprod/ods/nbs/nbs_disk_purchased_space"
    result_table_path = "//home/cloud_analytics/resources_overbooking/forecast-nbs"
    make_forecast_nbs(nbs_prod_used_path,
                      nbs_prod_purch_path,
                      nbs_preprod_used_path,
                      nbs_preprod_purch_path,
                      result_table_path)
