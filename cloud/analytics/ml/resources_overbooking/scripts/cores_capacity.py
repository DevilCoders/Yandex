# pylint: disable=no-value-for-parameter
import os
print(os.listdir('.'))
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

# import click
from spyt import spark_session
import logging.config
import datetime
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
import pandas as pd
import spyt
from clan_tools.utils.spark import SPARK_CONF_MEDIUM
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def extract_features(chunk_size: int = 100 * 2 ** 20):
    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_MEDIUM) as spark:
        spyt.info(spark)
        result_table = '//home/cloud_analytics/resources_overbooking/forecast'
        source_table = '//home/cloud_analytics/resources_overbooking/resource_monitoring'

        df = (
            spark.read
            .yt(source_table)
            .filter(col("node_name")!='all_nodes')
            .groupby(["platform", "metric", "start_time", "zone_id"])
            .agg(
                F.sum("min").alias("min"),
                F.sum("max").alias("max"),
                F.sum("avg").alias("avg"),
                F.sum("last").alias("last"),
                F.sum("sum").alias("sum")
            )
            .select("*", lit("all_nodes").alias("node_name"))
        )
        cols = ['node_name', 'zone_id', 'platform', 'start_time']

        predict_from_tmp = spark.read.yt(result_table).agg(F.max("training_date")).collect()[0][0]
        predict_from = (pd.to_datetime(predict_from_tmp*1e9) + datetime.timedelta(days=1)).strftime('%Y-%m-%d')
        predict_to = datetime.date.today().strftime('%Y-%m-%d')
        if (predict_from > predict_to):
            return None

        cores_free_all_min = (
            df
            .filter(col('node_name') == 'all_nodes')
            .filter(col('metric') == 'cores_free')
            .select(cols + [col('min').alias('cores_free_min')])
        )

        cores_total_all_min = (
            df
            .filter(col('node_name') == 'all_nodes')
            .filter(col('metric') == 'cores_total')
            .select(cols + [col('min').alias('cores_total_min')])
        )

        cores_used_all_max = (
            cores_total_all_min
            .join(cores_free_all_min, on=cols)
            .withColumn('cores_total_used', col('cores_total_min') - col('cores_free_min'))
            .cache()
        )

        # Для преобразования времени в дни
        seconds_in_day = 60*60*24

        cores_used_all_max_by_day = (
            cores_used_all_max
            .withColumn('start_day', F.floor(col('start_time')/seconds_in_day))
            .groupBy(['node_name', 'zone_id', 'platform', 'start_day'])
            .agg(F.min('cores_total_min').alias('cores_total_min'),
                 F.max('cores_total_used').alias('cores_total_used'),
                 F.max('start_time').alias('start_time'))
            .filter(col('zone_id') != 'all_zones')
            .select(cols+['cores_total_used', 'cores_total_min'])
            .cache()
        )

        cpu_platforms_list = ['standard-v1', 'standard-v2', 'standard-v3', 'unknown']

        all_cpu_platforms = (
            cores_used_all_max_by_day
            .filter(col('platform').isin(cpu_platforms_list))
            .groupBy('start_time', 'zone_id')
            .agg(F.sum('cores_total_min').alias('cores_total_min'), F.sum('cores_total_used').alias('cores_total_used'))
            .select([lit('all_nodes').alias('node_name'), 'zone_id', lit('all_cpu_platforms').alias('platform'),
                     'start_time', 'cores_total_used', 'cores_total_min'])
            .cache()
        )

        all_gpu_platforms = (
            cores_used_all_max_by_day
            .filter(~col('platform').isin(cpu_platforms_list))
            .groupBy('start_time', 'zone_id')
            .agg(F.sum('cores_total_min').alias('cores_total_min'), F.sum('cores_total_used').alias('cores_total_used'))
            .select([lit('all_nodes').alias('node_name'), 'zone_id', lit('all_gpu_platforms').alias('platform'),
                     'start_time', 'cores_total_used', 'cores_total_min'])
            .cache()
        )

        all_platforms = (
            cores_used_all_max_by_day
            .groupBy('start_time', 'zone_id')
            .agg(F.sum('cores_total_min').alias('cores_total_min'), F.sum('cores_total_used').alias('cores_total_used'))
            .select([lit('all_nodes').alias('node_name'), 'zone_id', lit('all_platforms').alias('platform'),
                     'start_time', 'cores_total_used', 'cores_total_min'])
            .cache()
        )

        all_zones = (
            cores_used_all_max_by_day
            .union(all_platforms)
            .union(all_cpu_platforms)
            .union(all_gpu_platforms)
            .groupBy('start_time', 'platform')
            .agg(F.sum('cores_total_min').alias('cores_total_min'), F.sum('cores_total_used').alias('cores_total_used'))
            .select([lit('all_nodes').alias('node_name'), lit('all_zones').alias('zone_id'),
                     'platform', 'start_time', 'cores_total_used', 'cores_total_min'])
            .cache()
        )

        cores_used_all_by_day = (
            cores_used_all_max_by_day
            .union(all_platforms)
            .union(all_cpu_platforms)
            .union(all_gpu_platforms)
            .union(all_zones)
            .cache()
        )

        # fix data skippings
        def hotfix(spark_df, fix_from, fix_to, days=20):
            calibr_from = (pd.to_datetime(fix_from+' 00:00:00') + datetime.timedelta(days=-days)).timestamp()
            fix_from = pd.to_datetime(fix_from + ' 00:00:00').timestamp()
            fix_to = pd.to_datetime(fix_to + ' 23:59:59').timestamp()

            temp_cols = ['node_name', 'zone_id', 'platform']
            calibr_df = (
                spark_df
                .filter(col('start_time') >= calibr_from)
                .filter(col('start_time') < fix_from)
            )
            calibr_coef = (
                calibr_df
                .groupBy(temp_cols)
                .agg(F.avg('cores_total_used').alias('fixed_used'),
                     F.avg('cores_total_min').alias('fixed_min'))
            )
            fix_df = (
                spark_df
                .filter(col('start_time')>=fix_from)
                .filter(col('start_time')<=fix_to)
                .join(calibr_coef, on=temp_cols, how='left')
                .select(cols+[col('fixed_used').alias('cores_total_used'), col('fixed_min').alias('cores_total_min')])
            )

            prev_df = spark_df[col('start_time') < fix_from]
            next_df = spark_df[col('start_time') > fix_to]
            final_df = prev_df.union(fix_df).union(next_df).toPandas()
            final_spdf = spark.createDataFrame(final_df)
            return final_spdf

        # fill skippings in period from 1st to 17th April, 2021
        cores_used_all_by_day = hotfix(spark_df=cores_used_all_by_day, fix_from='2020-11-19', fix_to='2020-11-21').cache()
        cores_used_all_by_day = hotfix(spark_df=cores_used_all_by_day, fix_from='2020-12-02', fix_to='2020-12-04').cache()
        cores_used_all_by_day = hotfix(spark_df=cores_used_all_by_day, fix_from='2021-04-01', fix_to='2021-04-17').cache()
        cores_used_all_by_day = hotfix(spark_df=cores_used_all_by_day, fix_from='2021-09-06', fix_to='2021-09-08').cache()
        cores_used_all_by_day = hotfix(spark_df=cores_used_all_by_day, fix_from='2021-11-30', fix_to='2021-12-02').cache()
        cores_used_all_by_day = hotfix(spark_df=cores_used_all_by_day, fix_from='2021-12-06', fix_to='2021-12-08').cache()
        logger.info('Final Dataframe built and fixed')

        schema_list = ['platform string',
                        'zone_id string',
                        'metric_name string',
                        'metric_value double',
                        'metric_upper_limit double',
                        'time long',
                        'mean double',
                        'mean_se double',
                        'mean_ci_lower double',
                        'mean_ci_upper double']

        res_udf_cols = [col.split()[0] for col in schema_list]
        def forecast_metric(ts_df):
            import datetime
            import statsmodels.api as sm

            ts_df = ts_df.sort_values(by='start_time')
            new_index = pd.to_datetime(ts_df.start_time, unit='s')
            ts_df = ts_df.set_index(new_index.values)
            future = (new_index[new_index.last_valid_index()] + datetime.timedelta(days=365*2))  # 2 years

            ts = ts_df.cores_total_used.resample('D').max().ffill()
            metric_upper_limit = ts_df.cores_total_min.resample('D').min().rename('metric_upper_limit').ffill()
            mod = sm.tsa.SARIMAX(ts, order=(1, 1, 1), trend='t')

            try:
                res = mod.fit(maxiter=200)
                result_df = res.get_forecast(future).summary_frame()
                result_df = pd.concat([ts.rename('metric_value'), metric_upper_limit, result_df], axis=1)
                result_df.metric_upper_limit = result_df.metric_upper_limit.ffill()
                result_df['time'] = result_df.index.astype(int)//10e9
                result_df['platform'] = ts_df['platform'].values[0]
                result_df['zone_id'] = ts_df['zone_id'].values[0]
                result_df['metric_name'] = 'cores_used'
            except Exception:
                result_df = pd.DataFrame(columns=res_udf_cols)
            return result_df

        dates = pd.date_range(start=predict_from, end=predict_to, freq='D').astype(int) // 10**9

        for i, date in enumerate(dates):
            logger.debug(f'Handling date {i+1}/{len(dates)} - {pd.to_datetime(date*1e9)}')
            results = (
                cores_used_all_by_day
                .filter(col('start_time')<=date)
                .groupBy('platform', 'zone_id')
                .applyInPandas(forecast_metric, schema=', '.join(schema_list))
                .withColumn('training_date', lit(date))
            )
            results.write.optimize_for("scan").yt(result_table, mode='append')

    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    YTAdapter(yt_token).yt.transform(result_table, desired_chunk_size=chunk_size)


if __name__ == '__main__':
    extract_features()
