import logging
from operator import mod

import pyspark.sql.functions as F
import pyspark.sql.types as T
from pyspark.sql.functions import broadcast, col, lit
from pyspark.sql.session import SparkSession
import pyspark.sql.dataframe as spd
from clan_tools.data_adapters.YTAdapter import YTAdapter

from clan_tools.data_adapters.crm.CRMHistoricalDataAdapter import CRMHistoricalDataAdapter


logger = logging.getLogger(__name__)


class SparkFeaturesAdapter:
    def __init__(self, spark: SparkSession, yt_adapter: YTAdapter):
        self._spark = spark
        self._yt_adapter = yt_adapter

    def get_day_use(self)->spd.DataFrame:
        spark = self._spark
        acq = spark.read.yt(
            "//home/cloud_analytics/cubes/acquisition_cube/cube")
        
        filter_condition = (col('event') == 'day_use') & (col('ba_usage_status') != 'service') \
            & ((col('segment') == 'Mass') | (col('segment') == 'Medium'))
        
        acq_day_use = acq.filter(filter_condition)

        ba_aggregates = acq_day_use \
            .groupBy('billing_account_id')\
            .agg(
                F.sum('real_consumption').alias('real_cons_sum'),
                F.sum('is_fraud').alias('is_fraud_sum'),
                F.max('event_time').alias('event_time')
            ).cache()

        non_zero_cons = ba_aggregates \
            .where(col('real_cons_sum') > 0)[['billing_account_id']]

        not_fraud = ba_aggregates \
            .where(col('is_fraud_sum') == 0)[['billing_account_id']]

        not_isv = acq_day_use \
            .join(ba_aggregates, on=['billing_account_id', 'event_time'])\
            .filter(col('is_isv') == 0)[['billing_account_id']].distinct()

        day_use = acq_day_use \
            .join(non_zero_cons, on=['billing_account_id'], how='inner')\
            .join(not_fraud, on=['billing_account_id'], how='inner')\
            .join(not_isv, on=['billing_account_id'], how='inner')\
            .select(['billing_account_id', 'real_consumption', 'event_time', 'ba_currency',
                     'ba_person_type', 'cloud_status', 'architect', 'ba_type', 'utm_source',  'account_name',
                     'database', 'balance', 'sku_name', 'service_name', 'total_visits',
                     'subservice_name', 'account_name', 'is_fraud', 'is_var', 'crm_account_owner',
                     'br_cost',        
                     'br_var_reward_rub',            
                     'br_credit_grant_rub',            
                     'br_credit_service_rub',       
                     'br_credit_cud_rub',             
                     'br_credit_volume_incentive_rub',
                     'br_credit_disabled_rub',         
                     'br_credit_trial_rub',           
                     'is_verified', 'sales_name', 'segment', 'phone', 'email'])\
            .withColumn('event_time', F.to_date('event_time'))
        return day_use.cache()

    def get_vm_data(self)->spd.DataFrame:
        spark = self._spark
        vm_cube = spark.read.yt("//home/cloud_analytics/compute_logs/vm_cube/vm_cube")\
            .groupBy(['ba_id', 'slice_time', 'vm_id'])\
            .agg(F.max('vm_cores').alias('cores'))\
            .select([F.to_date(F.from_unixtime(col('slice_time').cast(T.LongType()))).alias('slice_date'),
                     'vm_id', 'cores', col('ba_id').alias("billing_account_id")])
        vm_df = vm_cube.groupBy(['billing_account_id', 'slice_date'])\
            .agg(F.sum('cores').alias('sum_cores'),
                 F.min('cores').alias('min_cores'),
                 F.max('cores').alias('max_cores'),
                 F.count('vm_id').alias('vm_count'))\
            .select(['billing_account_id', col('slice_date').alias('event_time'),
                     'sum_cores', 'min_cores', 'max_cores', 'vm_count'])
        return vm_df.cache()

    def get_net_logs(self)->spd.DataFrame:
        acq = self._spark.read.yt(
            "//home/cloud_analytics/cubes/acquisition_cube/cube")
        net = self._spark.read.yt('//home/cloud_analytics/import/console_logs/events')\
            .withColumn("salt", (F.rand() * 200).cast(T.IntegerType()))
        acq = acq[acq.event == 'ba_created'].select(col('billing_account_id').alias('ba'), 'puid')\
            .withColumn("salt", (F.rand() * 200).cast(T.IntegerType()))
        net = acq.join(net, on=['puid', 'salt']).withColumn(
            'timestamp', F.to_date('timestamp'))
        return net.cache()

    
    def get_crm_leads(self):
        crm_leads = CRMHistoricalDataAdapter(self._yt_adapter, self._spark).historical_preds()
        model_leads = crm_leads.filter(col('lead_source').like('%30k%')|col('lead_source').like('%50k%'))
        leads_labels = (model_leads
            .filter((col('status') == 'Recycled') 
                    | (col('status') =='Converted'))
            .withColumn('lead_label', F.when(col('status') == 'Recycled', 0).otherwise(1))
            .groupby('billing_account_id')
            .agg(F.max('lead_label').alias('lead_label')))
        return leads_labels

