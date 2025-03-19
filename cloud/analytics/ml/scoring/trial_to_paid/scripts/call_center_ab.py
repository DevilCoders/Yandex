import os
import click
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import DEFAULT_SPARK_CONF
from spyt import spark_session
import logging.config

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"


@click.command()
@click.option('--result_path')
def call_center_ab(result_path):
    with spark_session(yt_proxy="hahn", spark_conf_args=DEFAULT_SPARK_CONF, driver_memory='2G') as spark:

        leads_cube = (
            spark.read.yt('//home/cloud_analytics/import/crm/leads/leads_cube')
            .filter(col('lead_source')=='trial')
            .filter(col('lead_source_description') == 'Client is Individual')
            .cache()
        )

        cube_path = '//home/cloud_analytics/cubes/acquisition_cube/cube'

        def max_by(x, y):
            return F.expr(f'max_by({x}, {y})')

        leads_statuses = (
            leads_cube.groupby('billing_account_id')
            .agg(max_by('status', 'date_modified').alias('lead_status'))
        )

        leads_groups = (
            spark.read.yt('//home/cloud_analytics/scoring_v2/ab_test')
            .select(
                col('ba_id').alias('billing_account_id'),
                F.to_date('scoring_date').alias('scoring_date'),
                col('group')
            )
            .distinct()
            .cache()
        )

        calls_info = (
            spark.read
            .yt('//home/cloud_analytics/lunin-dv/crm/crm_call_infromation_for_billings')
            .filter(col('lead_source')=='trial')
            .filter(col('lead_source_description') == 'Client is Individual')
            .cache()
        )

        cube = spark.read.yt(cube_path)
        had_calls = (
            calls_info
            .select('billing_account_id', lit(1).alias('had_call'), col('status').alias('call_status'))
            .distinct()
        )

        individual_accs = (
            cube
            .filter(col('event')=='ba_created')
            .filter(col('ba_usage_status')!='service')
            .filter(col('ba_person_type') == 'individual')
            .filter(col('ba_usage_status')!='trial')
            .select('billing_account_id')
            .distinct()
        )

        consumption = (
            cube
            .filter(col('event')=='day_use')
            .select(
                'billing_account_id',
                F.to_date('event_time').alias('event_date'),
                'real_consumption',
                'br_cost'
            )
        )

        days_to_now = F.datediff(F.current_date(), F.min('scoring_date'))
        leads_consumption_by_ba = (
            consumption
            .join(leads_groups, on='billing_account_id')
            .join(individual_accs, on='billing_account_id')
            .join(had_calls, on='billing_account_id', how='left')
            .join(leads_statuses, on='billing_account_id', how='left')
            .filter(((col('had_call')==1) & (col('group')=='test')) | (col('had_call').isNull() & (col('group')=='control')))
            .select('billing_account_id',
                    'event_date',
                    'real_consumption',
                    'br_cost',
                    'group',
                    'scoring_date',
                    'call_status',
                    'lead_status',
                    F.datediff('event_date', 'scoring_date').alias('days_from_scoring'))
            .filter(col('days_from_scoring') > 1)
            .filter(col('days_from_scoring') <= 60)
            .groupby('billing_account_id', 'group')
            .agg((
                F.sum('real_consumption')/days_to_now).alias('paid'),
                (F.sum('br_cost')/days_to_now).alias('mean_br_cost'),
                (F.max((col('call_status')=='Held').cast('int'))).alias('held_count'),
                (F.max((col('lead_status')=='Converted').cast('int'))).alias('converted_count')
            )
        ).cache()

        leads_consumption = (
            leads_consumption_by_ba
            .groupby('group')
            .agg(
                F.mean('paid').alias('mean_paid'),
                F.mean('mean_br_cost').alias('mean_br_cost'),
                F.countDistinct('billing_account_id').alias('accounts'),
                F.sum((col('paid')>0).cast('int')).alias('count_paid'),
                F.sum(col('held_count')).alias('held_count'),
                F.sum(col('converted_count')).alias('converted_count'),
            )
            .select(
                F.unix_timestamp().alias('updated_at'),
                '*',
                (col('count_paid')/col('accounts')).alias('paid_ratio')
            )
        )

        leads_consumption.write.yt(result_path, mode='append')


if __name__ == '__main__':
    call_center_ab()
