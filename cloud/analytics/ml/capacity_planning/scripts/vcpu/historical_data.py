# pylint: disable=no-value-for-parameter
import os
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import yaml
import click
import numpy as np
import pandas as pd
import logging.config
import spyt
from spyt import spark_session
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from datetime import datetime, timedelta
from clan_tools.utils.spark import SPARK_CONF_MEDIUM
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--env')
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--repair', is_flag=True, default=False)
def collect_ts(env: str, is_prod: bool, repair: bool) -> None:
    """Prepare dataset for forecasting and fix data in bad periods

    Args:
        env (str): environment of resources (must be one of `prod` or `preprod`)
        is_prod (bool): if set to False workflow works with test tables and sources (used for debug)
        repair (bool): if True starts repairing part according to config periods

    """

    logger.info(f'Preprocess {env} data about vCPU...')
    yt_token = os.environ["SPARK_SECRET"] if "SPARK_SECRET" in os.environ else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)

    prod_postfix = '' if is_prod else '_test'
    source_path = f'//home/cloud_analytics/ml/capacity_planning/vcpu/import/{env}/all'
    result_table = f'//home/cloud_analytics/ml/capacity_planning/vcpu/historical_data_{env}{prod_postfix}'
    backup_path = f'//home/cloud_analytics/ml/capacity_planning/vcpu/backups/historical_data/{env}'
    end_dt = (datetime.now() - timedelta(days=1)).strftime('%Y-%m-%d')

    # spark session
    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_MEDIUM) as spark:
        spyt.info(spark)

        cpu_platforms_list = ['standard-v1', 'standard-v2', 'standard-v3', 'unknown']
        colnames = ['rep_date', 'platform', 'zone', 'used_wop', 'used', 'total']
        spdf_used_total = spark.read.yt(source_path).select(colnames)

        all_cpu_platforms = (
            spdf_used_total
            .filter(col('platform').isin(cpu_platforms_list))
            .groupby('rep_date', 'zone')
            .agg(
                F.sum('used_wop').alias('used_wop'),
                F.sum('used').alias('used'),
                F.sum('total').alias('total')
            )
            .withColumn('platform', lit('all_cpu_platforms'))
            .select(colnames)
        )

        all_gpu_platforms = (
            spdf_used_total
            .filter(~col('platform').isin(cpu_platforms_list))
            .groupby('rep_date', 'zone')
            .agg(
                F.sum('used_wop').alias('used_wop'),
                F.sum('used').alias('used'),
                F.sum('total').alias('total')
            )
            .withColumn('platform', lit('all_gpu_platforms'))
            .select(colnames)
        )

        all_platforms = (
            spdf_used_total
            .groupby('rep_date', 'zone')
            .agg(
                F.sum('used_wop').alias('used_wop'),
                F.sum('used').alias('used'),
                F.sum('total').alias('total')
            )
            .withColumn('platform', lit('all_platforms'))
            .select(colnames)
        )

        all_zones = (
            spdf_used_total
            .union(all_platforms)
            .union(all_cpu_platforms)
            .union(all_gpu_platforms)
            .groupby('rep_date', 'platform')
            .agg(
                F.sum('used_wop').alias('used_wop'),
                F.sum('used').alias('used'),
                F.sum('total').alias('total')
            )
            .withColumn('zone', lit('all_zones'))
            .select(colnames)
        )

        spdf_final = (
            spdf_used_total
            .union(all_platforms)
            .union(all_cpu_platforms)
            .union(all_gpu_platforms)
            .union(all_zones)
            .withColumn('rep_date', F.date_format('rep_date', 'yyyy-MM-dd'))
            .coalesce(1)
        )

        if repair:
            logger.info('Repairing timeseries')
            fix_dates_path = '//home/cloud_analytics/ml/capacity_planning/vcpu/cfg/fix_dates.yaml'
            fix_dates_cfg = yt_adapter.yt.read_file(fix_dates_path).read()
            fix_periods = yaml.safe_load(fix_dates_cfg)['fix_periods']

            def fix_dts(df):
                tdf = df.sort_values('rep_date')
                for period in fix_periods:
                    fix_end = min(period['end'], end_dt)
                    if fix_end not in tdf['rep_date']:
                        tdf = pd.concat([tdf, pd.DataFrame([{'rep_date': fix_end}])])
                tdf['rep_date'] = pd.to_datetime(tdf['rep_date'])
                tdf = tdf.set_index('rep_date')
                tdf = tdf.resample('1D').agg(lambda x: x[0] if len(x) > 0 else np.nan)
                free_dts = (tdf['total'] - tdf['used']).ffill()
                free_wop_dts = (tdf['total'] - tdf['used_wop']).ffill()
                for period in fix_periods:
                    if (period['start']) > tdf.index.min().strftime('%Y-%m-%d'):
                        bool_ind = ((period['start']) <= tdf.index.to_series()) & (tdf.index.to_series() <= (period['end']))
                        tdf.loc[bool_ind, 'total'] = np.nan
                tdf['total'] = tdf['total'].ffill()
                tdf['used'] = tdf['total'] - free_dts
                tdf['used_wop'] = tdf['total'] - free_wop_dts
                tdf = tdf.ffill().reset_index()
                tdf['rep_date'] = tdf['rep_date'].dt.strftime('%Y-%m-%d')
                tdf = tdf[['rep_date', 'platform', 'zone', 'used_wop', 'used', 'total']].dropna()
                return tdf

            schema_list = ['rep_date string', 'platform string', 'zone string', 'used_wop double', 'used double', 'total double']
            repaired_spdf = spdf_final.groupby('platform', 'zone').applyInPandas(fix_dts, schema=', '.join(schema_list))
            repaired_spdf.write.yt(result_table, mode='overwrite')
            if is_prod:
                repaired_spdf.write.yt(f'{backup_path}/historical_data_{end_dt}', mode='overwrite')
        else:
            spdf_final.write.yt(result_table, mode='overwrite')
            if is_prod:
                spdf_final.write.yt(f'{backup_path}/historical_data_{end_dt}', mode='overwrite')

    yt_adapter.optimize_chunk_number(result_table)
    yt_adapter.optimize_chunk_number(f'{backup_path}/historical_data_{end_dt}')
    yt_adapter.leave_last_N_tables(backup_path, 7)

if __name__ == '__main__':
    collect_ts()
