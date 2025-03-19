
from spyt import spark_session
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.crm.CRMModelAdapter import CRMModelAdapter, upsale_to_update_leads
from pyspark.sql.functions import col, lit
from clan_tools.utils.spark import DEFAULT_SPARK_CONF
from clan_tools.logging.logger import default_log_config
import pyspark.sql.functions as F
import click
from datetime import datetime, timedelta
import logging.config
import os

os.environ["JAVA_HOME"] = "/usr/local/jdk-11"


logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


def description(time_from: datetime):
    return F.concat(lit('Account "'),
                    col('billing_account_id'),
                    lit('" started first trial consumption at'),
                    lit(f' {time_from.isoformat()}. '),
                    lit('Since then it has '),
                    F.round(col('trial_consumption'), 2),
                    lit(' rub trial and '),
                    F.round(col('real_consumption'), 2),
                    lit(' rub paid consumption '),
                    ).alias('description')


@click.command()
@click.option('--cloud_cube')
@click.option('--crm_path')
@click.option('--leads_daily_limit')
def leads(cloud_cube: str,  crm_path: str, leads_daily_limit=1000):
    with spark_session(yt_proxy="hahn",
                       spark_conf_args=DEFAULT_SPARK_CONF,
                       driver_memory='2G') as spark:
        yt_adapter = YTAdapter(token=os.environ["SPARK_SECRET"])

        def get_trial_companies_leads(ba_person_type_actual_expr, time_from):
            return (
                spark
                .read
                .yt(cloud_cube)
                .withColumn('event_time', F.date_trunc('day', F.to_date('event_time')))
                .filter(col('event') == 'day_use')
                .filter(col('is_isv') == 0)
                .filter(col('is_fraud') == 0)
                .filter(col('trial_consumption') > 0)
                .filter(col('ba_person_type_actual').contains('company'))
                .filter(col('ba_usage_status_actual') == 'trial')
                .filter(col('crm_account_segment') == 'Mass')
                .filter(col('is_var') != 'var')
                .filter(ba_person_type_actual_expr)
                .groupby('billing_account_id')
                .agg(
                    F.sum('br_cost').alias('br_cost'),
                    F.sum('real_consumption').alias('real_consumption'),
                    F.sum('trial_consumption').alias('trial_consumption'),
                    F.min('event_time').alias('event_time_min'),
                    F.max('event_time').alias('event_time_max'),
                )
                .filter(col('event_time_min') == time_from)
                .select(
                    'billing_account_id',
                    col('real_consumption').alias('sort_column'),
                    description(time_from)
                )
                .cache()
            )

        resident_adapter = CRMModelAdapter(
            yt_adapter, spark,
            predictions=get_trial_companies_leads(
                col('ba_person_type_actual') != 'switzerland_nonresident_company',
                (datetime.now() - timedelta(days=16)).date()
            ),
            leads_daily_limit=int(leads_daily_limit),
            lead_source='TrialCompanies'
        )
        resident_leads, _ = resident_adapter.save_to_crm()

        (upsale_to_update_leads(resident_leads, 'trial')
            .write.yt(crm_path, mode='append'))

        nonresident_adapter = CRMModelAdapter(
            yt_adapter, spark,
            predictions=get_trial_companies_leads(
                col('ba_person_type_actual') == 'switzerland_nonresident_company',
                (datetime.now() - timedelta(days=2)).date()
            ),
            leads_daily_limit=int(leads_daily_limit),
            lead_source='TrialCompaniesNonresidents'
        )
        nonresident_leads, _ = nonresident_adapter.save_to_crm()

        (
            upsale_to_update_leads(nonresident_leads, 'trial')
            .write.yt(crm_path, mode='append')
        )


if __name__ == '__main__':
    leads()
