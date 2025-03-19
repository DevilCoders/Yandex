import logging.config
import os

import click
from pyspark import java_gateway
import pyspark.sql.dataframe as spd
import pyspark.sql.functions as F
import pyspark.sql.types as T
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import DEFAULT_SPARK_CONF
from pyspark.sql.functions import array_distinct, col, flatten, lit, split, udf
from spyt import spark_session
from datetime import datetime, timedelta
from clan_tools.data_adapters.YTAdapter import YTAdapter
import os
from itertools import chain



os.environ["JAVA_HOME"] = "/usr/local/jdk-11"


logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


def make_onboarding_leads(cloud_node_info, clouds_created, spark_nodes_info,  spark_clouds, 
        cloud_crm_acounts):

    spark_no_clouds = (
        spark_clouds
        .join(cloud_node_info, on='passport_id', how='left')
        .join(clouds_created, on='passport_id', how='left')
        .groupby('spark_id')
        .agg((F.sum('managed') > 0).cast('int').alias('managed'),
             (F.sum('visitted') > 0).cast('int').alias('visitted'),
             F.sum(F.coalesce(col('real_consumption'), lit(0))).alias('real_consumption'),
             F.sum(F.coalesce(col('trial_consumption'), lit(0))).alias('trial_consumption'),
             F.sum(F.coalesce(col('br_cost'), lit(0))).alias('br_cost'),
             F.collect_set('passport_id').alias('passport_ids'),
             F.collect_set('billing_account_id').alias('billing_account_ids'),
             F.array_distinct(F.flatten(F.collect_list('account_names'))).alias('account_names'),
             F.array_distinct(F.flatten(F.collect_list('cloud_ids'))).alias('cloud_ids'),
             F.min('first_cloud_created_at').alias('first_cloud_created_at'),
             F.min('last_cloud_created_at').alias('last_cloud_created_at'),
             (F.sum(1-col('just_cloud')) == 0).cast('int').alias('just_cloud'),
             F.max(F.coalesce('level', lit(-1))).alias('max_level'),
             F.min(F.coalesce('level', lit(-1))).alias('min_level'),
        )
        
    )
     
    
    
    onboarding_leads = (
        spark_nodes_info
        .join(spark_no_clouds, on='spark_id', how='left') 
        .join(cloud_crm_acounts, on='inn', how='left')
        .distinct()
            # .filter(
            #     ((col('managed') == False) & (col('real_consumption') == 0))
            #     | F.isnull('real_consumption')
            # )
    )
    
    all_columns = onboarding_leads.columns
    except_cols  = {
        'min_level',
        'max_level',
        'egrul_likvidation',
        'visitted'
    }

    same_columns = [col for col in all_columns if col not in except_cols]

    onboarding_leads_additional = (
        onboarding_leads
          .select(
            *same_columns,
            F.coalesce(col('egrul_likvidation'), lit('')).alias('egrul_likvidation'),
            (-1*col('company_size_revenue')).alias('sort_col'),
            F.size(col('phones_list')).alias('spark_phones_count'),
            F.size(col('passport_ids')).alias('puids_count'),
            (
                (F.size(F.coalesce(col('billing_account_ids'), F.array([])))
                + F.size(F.coalesce(col('cloud_ids'), F.array([])))
                + F.coalesce(col('inn_in_cloud'), lit(0))) > 0
            ).cast('int').alias('match'),
            F.coalesce(col('segment')=='Enterprise', lit(False)).cast('int').alias('segment_enterprise'),
            F.coalesce(col('segment')=='Medium', lit(False)).cast('int').alias('segment_medium'),
            F.coalesce(col('segment')=='Mass', lit(False)).cast('int').alias('segment_mass'),
            F.coalesce(col('min_level'), lit(-1)).alias('min_level'),
            F.coalesce(col('max_level'), lit(-1)).alias('max_level'),
            F.coalesce(col('visitted'), lit(0)).alias('visitted'),
            F.coalesce((col('max_level') - col('min_level')), lit(0)).alias('levels_diff'),
            (F.coalesce(col('domain'), lit(''))!='').cast('int').alias('has_domain_in_spark'),
            F.split(F.coalesce(col('main_okved2_code'), lit('0')), '\.').getItem(0).cast('int').alias('first_okved')
        )
        
    ).cache()


    return onboarding_leads_additional


def make_csm_leads(cloud_node_info, spark_nodes_info,  spark_clouds, cloud_crm_acounts):
    unmanaged_spark_id = (
        spark_clouds
        .join(cloud_node_info, on='passport_id')
        .groupby('spark_id')
        .agg(
            (F.sum('managed') > 0).cast('int').alias('managed'),
            F.sum('real_consumption_180').alias('real_consumption_180'),
            F.collect_list('passport_id').alias('passport_ids'),
        )
        .filter((col('managed') == False) & (col('real_consumption_180') > 0))
    )

    csm_leads = (
        spark_nodes_info
        .filter(col('company_size_description') != 'Микропредприятие')
        .join(unmanaged_spark_id, on='spark_id')
        .sort(col('company_size_revenue') > 0)
        .join(cloud_crm_acounts, on='inn', how='left_anti')

    ).cache()

    leads_bas = (
        csm_leads
        .select('spark_id',
                'inn',
                'spark_name',
                'company_size_revenue',
                'workers_range',
                F.explode('passport_ids').alias('passport_id'))
        .join(cloud_node_info, on='passport_id')
        .join(spark_clouds, on=['spark_id', 'passport_id'])
        .withColumn('sort_col', -1*col('company_size_revenue'))
    )

    return leads_bas



def write_sorted(df: spd.DataFrame, path: str):
    (
        df
        .sort('sort_col')
        .write
        .sorted_by('sort_col')
        .mode("overwrite").yt(path)
    )


@click.command()
@click.option('--cloud_info')
@click.option('--spark_info')
@click.option('--paths_dir')
@click.option('--leads_dir')
@click.option('--crm_accounts')
def leads(cloud_info: str, spark_info: str, crm_accounts: str, paths_dir: str, leads_dir: str):
    with spark_session(yt_proxy="hahn", spark_conf_args=DEFAULT_SPARK_CONF, driver_memory='2G') as spark:
        yt_adapter = YTAdapter(token=os.environ['SPARK_SECRET'])


        managed = ((F.coalesce(col('crm_account_owner_current'), lit('')) != 'No Account Owner')
                   & (F.coalesce(col('crm_account_owner_current'), lit('')) != '')
                   & (F.coalesce(col('crm_account_segment_current'), lit('Mass')) != 'Mass')
                   ).cast("long").alias('managed')


        last_180 = ((F.to_date(col('event_time')) > datetime.now().date() - timedelta(days=180))
                    & (F.col("event") == F.lit("day_use")))

        clouds_created = (
            spark.read.yt('//home/cloud_analytics/import/iam/cloud_owners_history')
            .withColumn('just_cloud', (col('cloud_status')!='CREATING').cast('int'))
            .withColumnRenamed('passport_uid', 'passport_id')
            .groupBy('passport_id')
            .agg(F.collect_set('cloud_id').alias('cloud_ids'),
                 F.min('cloud_created_at').alias('first_cloud_created_at'),
                 F.max('cloud_created_at').alias('last_cloud_created_at'),
                 (F.sum('just_cloud') == 0).cast('int').alias('just_cloud')
            )
        )

        cloud_node_info = (
            spark.read.yt(cloud_info)
            .filter(col('billing_account_id')!='')
            .select(
                col('puid').alias('passport_id'),
                col('billing_account_id'),
                col('account_name'),
                managed,
                col('real_consumption'),
                col('trial_consumption'),
                col('br_cost'),
                F.when(F.col("event") == F.lit("visit"),
                       1).otherwise(0).alias("visitted"),
                F.when(last_180, col('real_consumption')).otherwise(
                    0).alias("real_consumption_180"),
                F.when(last_180, col('trial_consumption')).otherwise(
                    0).alias("trial_consumption_180"),
            )
            .groupby(['passport_id', 'billing_account_id'])
            .agg(
                F.collect_set('account_name').alias('account_names'),
                F.sum('br_cost').alias('br_cost'),
                F.sum('real_consumption').alias('real_consumption'),
                F.sum('trial_consumption').alias('trial_consumption'),
                F.sum('real_consumption_180').alias('real_consumption_180'),
                F.sum('trial_consumption_180').alias('trial_consumption_180'),
                (F.sum('managed') > 0).cast('int').alias('managed'),
                (F.sum('visitted') > 0).cast('int').alias('visitted')
            )
            .cache()
        )


        spark_nodes_info = (
            spark.read
            .schema_hint({'phones_list': T.ArrayType(T.StringType())})
            .yt(spark_info)
            .select(col('spark_id'),
                    col('inn'),
                    col('name').alias('spark_name'),
                    'email',
                    'phones_list',
                    'company_size_revenue',
                    'company_size_description',
                    'company_size_actual_date',
                    'legal_city',
                    'legal_region',
                    'okato_region_name',
                    'workers_range',
                    'domain',
                    'egrul_likvidation',
                    'main_okved2_name',
                    'main_okved2_code')
            .filter(col('company_size_revenue').isNotNull())
        )

        passport_id2spark_id = (
            spark.read
            .schema_hint(
                {
                    'src':  T.StringType(),
                    'dst': T.StringType(),
                    'path': T.ArrayType(T.StringType())
                }
            )
            .yt('//home/cloud_analytics/scoring_of_potential/clean_id_graph/passport_id2spark_id')
        )

        spark_clouds = (
            passport_id2spark_id
            .select(
                F.split('src', '::')[1].alias('passport_id'),
                F.split('dst', '::')[1].alias('spark_id'),
                (F.size('path')-2).alias('level')
            )
        ).cache()

        segment_order = {
            'Mass':0,
            'Medium':1,
            'Enterprise':2
        }

        mapping_expr = F.create_map([lit(x) for x in chain(*segment_order.items())])
        def max_by(x, y):
            return F.expr(f'max_by({x}, {y})')
        def trim(col, trim_str):
            return F.expr(f"trim(BOTH '{trim_str}' FROM {col})")

        cloud_crm_acounts = (
            spark.read
            .yt(yt_adapter.last_table_name(crm_accounts))
            .filter(col('deleted')==False)
            .select(col('inn'), 
                    'segment', 
                    mapping_expr[col('segment')].alias('segment_order'),
                    F.split(trim('industry', '^'), '\^,\^').alias('industries'))
            .groupby('inn')
            .agg(
                max_by('segment', 'segment_order').alias('segment'),
                max_by('industries', 'segment_order').alias('crm_industries'),
            )
            .select('inn', 'segment',  'crm_industries', lit(1).alias('inn_in_cloud'))
        ).cache()

        onboarding_leads = make_onboarding_leads(
            cloud_node_info, clouds_created, spark_nodes_info,  spark_clouds, cloud_crm_acounts)

        write_sorted(onboarding_leads, f'{leads_dir}/onboarding_leads')

        csm_leads = make_csm_leads(
            cloud_node_info, spark_nodes_info,  spark_clouds, cloud_crm_acounts)

        write_sorted(csm_leads, f'{leads_dir}/csm_leads')


if __name__ == '__main__':
    leads()
