import os
import logging.config
import pandas as pd
from datetime import timedelta
import spyt
from spyt import spark_session
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.utils.spark import DEFAULT_SPARK_CONF

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def load_data_kkr_nrd(spark: spark_session,
                      kkr_prod_path: str,
                      kkr_preprod_path: str,
                      nrd_prod_path: str,
                      KB_in_B: int = 1024):
    logger.info('Starting to load DataFrame')

    df_kkr_prod = spark.read.yt(kkr_prod_path).withColumn('env', lit('prod'))
    df_kkr_preprod = spark.read.yt(kkr_preprod_path).withColumn('env', lit('preprod'))

    df_kkr = (
        df_kkr_prod.union(df_kkr_preprod)
        .select(
            F.to_timestamp('created_timestamp').alias('time'),
            F.to_date(F.to_timestamp('created_timestamp')).alias('date'),
            'env',
            F.when(col('media')=='rot', lit('hdd')).otherwise(col('media')).alias('disk_type'),
            F.substring(col('cluster'), -3, 3).alias('datacenter'),
            F.substring(col('sensor'), 0, 4).alias('metric'),
            'value'
        )
        .filter(col('datacenter').isin(['sas', 'myt', 'vla']))
        .select(
            '*',
            F.when(col('metric')=='Used', col('value')).alias('kkr_used'),
            F.when(col('metric')=='Free', col('value')).alias('kkr_free')
        )
        .groupby('date', 'time', 'env', 'disk_type', 'datacenter')
        .agg(
            (F.max('kkr_used')/KB_in_B**4).alias('used_TB'),
            ((F.max('kkr_free')+F.max('kkr_used'))/KB_in_B**4).alias('total_TB')
        )
        .select(lit('kikimr').alias('source'), '*')
        .groupby(['source', 'date', 'env', 'disk_type', 'datacenter'])
        .agg(F.max('used_TB').alias('used_TB'), F.max('total_TB').alias('total_TB'))
        .cache()
    )

    df_nrd_prod = spark.read.yt(nrd_prod_path).withColumn('env', lit('prod'))
    # one slot of 1.6 Gb size is used to make 15 chunks of size 93 Mb
    nrd_kf = (1.6*1024)/(15*93)
    df_nrd = (
        df_nrd_prod
        .select(
            F.to_timestamp('created_timestamp').alias('time'),
            F.to_date(F.to_timestamp('created_timestamp')).alias('date'),
            'env',
            col('cluster').alias('datacenter'),
            lit('ssd_nrd').alias('disk_type'),
            col('sensor').alias('metric'),
            'value',
            F.when(col('sensor')=='FreeBytes', col('value')).alias('nrd_free'),
            F.when(col('sensor')=='TotalBytes', col('value')).alias('nrd_total')
        )
        .groupby(['date', 'time', 'env', 'disk_type', 'datacenter'])
        .agg(
            (nrd_kf*(F.min('nrd_total')-F.min('nrd_free'))/KB_in_B**4).alias('used_TB'),
            (nrd_kf*F.max('nrd_total')/KB_in_B**4).alias('total_TB')
        )
        .withColumn('source', lit('nrd'))
        .groupby('source', 'date', 'env', 'disk_type', 'datacenter')
        .agg(F.max('used_TB').alias('used_TB'), F.max('total_TB').alias('total_TB'))
        .cache()
    )

    df = df_kkr.union(df_nrd)
    df_all = (
        df.groupby('source', 'env', 'disk_type', 'date')
        .agg(F.sum('used_TB').alias('used_TB'), F.sum('total_TB').alias('total_TB'))
        .select('source', 'date', 'env', 'disk_type', lit('all').alias('datacenter'), 'used_TB', 'total_TB')
    )
    df_result = df.union(df_all).cache()
    return df_result


def generate_default_params():
    logger.info('Default training params generating')
    def_sarimax_params = {'p': 1, 'd': 1, 'q': 1, 'P': 0, 'D': 0, 'Q': 0, 'S': 0, 'trend': 't'}
    disk_types = ['ssd', 'ssd_nrd', 'hdd']
    datacenters = ['all', 'myt', 'vla', 'sas']
    default_params = {}
    for env in ['prod', 'preprod']:
        default_params[env] = {}
        for dtp in disk_types:
            default_params[env][dtp] = {}
            for dtc in datacenters:
                default_params[env][dtp][dtc] = {}
                default_params[env][dtp][dtc]['sarimax'] = def_sarimax_params
                default_params[env][dtp][dtc]['from_date'] = '2015-01-01'
                default_params[env][dtp][dtc]['days_to_forecast'] = 200
                default_params[env][dtp][dtc]['days_in_history'] = 180
                default_params[env][dtp][dtc]['periods_to_ignore'] = []
    return default_params


def make_forecast_kkr_nrd(kkr_prod_path: str,
                          kkr_preprod_path: str,
                          nrd_prod_path: str,
                          result_table_path: str,
                          predict_from: str = None,
                          predict_to: str = None,
                          params: dict = None,
                          spark_conf: dict = DEFAULT_SPARK_CONF,
                          chunk: int = 200*1024**2):
    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    yta = YTAdapter(yt_token).yt
    #  spark session
    with spark_session(yt_proxy="hahn", spark_conf_args=spark_conf) as spark:
        spyt.info(spark)
        df = load_data_kkr_nrd(spark, kkr_prod_path, kkr_preprod_path, nrd_prod_path)

        if (predict_from is None):
            result_table_name = result_table_path.split('/')[-1]
            result_table_folder = '/'.join(result_table_path.split('/')[:-1])
            if result_table_name in yta.list(result_table_folder):
                predict_from_tmp = (
                    spark.read.yt(result_table_path).select("training_date").agg(F.max("training_date")).collect()[0][0]
                )
                predict_from = (pd.to_datetime(predict_from_tmp) + timedelta(days=1)).strftime('%Y-%m-%d')
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
        def forecast_metric(ts_df, params=params, date_col="date", val_col="used_TB", lim_col="total_TB"):
            import datetime
            import statsmodels.api as sm
            from numpy import nan
            def transform_ignore(df, ignore_periods=[], date_col: str = None):
                if len(ignore_periods) == 0:
                    return df
                df = df.copy()
                if not (date_col is None):
                    df = df.set_index(date_col)
                for event in ignore_periods:
                    start_ = pd.to_datetime(event['from_date'])
                    days_delta_ = datetime.timedelta(days=event['num_days'])
                    end_ = start_ + days_delta_
                    if (start_ > df.index.max()) or (end_ < df.index.min()):
                        continue
                    start_ = max(df.index.min(), start_)
                    end_ = min(df.index.max(), end_)
                    delta_ = df.loc[end_] - df.loc[start_]
                    df_start_ = df[df.index<start_] + delta_
                    df_start_.index = df_start_.index + days_delta_
                    df_end_ = df[df.index>=end_]
                    df = pd.concat([df_start_, df_end_], axis=0)
                if not (date_col is None):
                    df = df.reset_index()
                return df

            ts_df = ts_df.sort_values(by=date_col)
            ind = pd.to_datetime(ts_df[date_col])
            ts_df = ts_df.set_index(ind)
            ts_df = ts_df.resample('D').max().ffill()
            ts_df[date_col] = ts_df.index.astype(str)
            env, dtp, dtc = (ts_df['env'].values[-1], ts_df['disk_type'].values[-1], ts_df['datacenter'].values[-1])
            future = (ind[ind.last_valid_index()] + datetime.timedelta(days=params[env][dtp][dtc]['days_to_forecast']))

            ts_df['comment'] = nan
            for event in params[env][dtp][dtc]['periods_to_ignore']:
                start_ = pd.to_datetime(event['from_date'])
                end_ = start_ + datetime.timedelta(days=event['num_days'])
                bool_ind = (ts_df.index >= start_) & (ts_df.index < end_)
                ts_df.loc[bool_ind, 'comment'] = event['comment']

            try:
                from_date = pd.to_datetime(params[env][dtp][dtc]['from_date'])
                ts = ts_df[val_col]
                ts = ts[ts.index>from_date]
                ts = ts[-params[env][dtp][dtc]['days_in_history']:]
                ts = transform_ignore(ts, ignore_periods=params[env][dtp][dtc]['periods_to_ignore'])
                fit_params = params[env][dtp][dtc]['sarimax']
                model = sm.tsa.SARIMAX(ts, order=(fit_params['p'], fit_params['d'], fit_params['q']),
                                       seasonal_order=(fit_params['P'], fit_params['D'],
                                                       fit_params['Q'], fit_params['S']),
                                       trend=fit_params['trend'])
                res = model.fit(maxiter=500)
                result_df = res.get_forecast(future).summary_frame()
                result_df = result_df.clip(lower=0)
                result_df = pd.concat([ts_df, result_df], axis=1)
                result_df[date_col] = result_df.index.astype(str)
                cols_to_ffill = ['source', 'env', 'disk_type', 'datacenter', lim_col]
                result_df[cols_to_ffill] = result_df[cols_to_ffill].ffill()

            except Exception:
                logger.info(f'Can not make forecast for {env}, {dtp}, {dtc}')
                result_df = ts_df.copy()
                result_df['mean'] = nan
                result_df['mean_se'] = nan
                result_df['mean_ci_lower'] = nan
                result_df['mean_ci_upper'] = nan
            result_df = result_df.reset_index(drop=True)
            return result_df

        # calculation
        schema_list = [
            'source string',
            'env string',
            'disk_type string',
            'datacenter string',
            'date string',
            'used_TB double',
            'total_TB double',
            'comment string',
            'mean double',
            'mean_se double',
            'mean_ci_lower double',
            'mean_ci_upper double'
        ]

        for i, date in enumerate(dates):
            logger.info('Iter. %3d: Prediction for date %s' % (i+1, date))
            dff = (
                df.filter(col('date')<=date).groupby(["source", "env", "disk_type", "datacenter"])
                .applyInPandas(lambda x: forecast_metric(ts_df=x), schema=', '.join(schema_list))
                .withColumn('training_date', lit(date))
            )
            dff.write.optimize_for("scan").yt(result_table_path, mode='append')
    logger.info('Result table optimization')
    yta.transform(result_table_path, desired_chunk_size=chunk, optimize_for="scan")


if __name__ == '__main__':
    kkr_prod_path = "//home/cloud_analytics/dwh/ods/nbs/kikimr_disk_used_space"
    kkr_preprod_path = "//home/cloud_analytics/dwh_preprod/ods/nbs/kikimr_disk_used_space"
    nrd_prod_path = "//home/cloud_analytics/dwh/ods/nbs/nbs_nrd_used_space"
    result_table_path = "//home/cloud_analytics/resources_overbooking/forecast-kkr-nrd"
    make_forecast_kkr_nrd(kkr_prod_path, kkr_preprod_path, nrd_prod_path, result_table_path)
