import time
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import SPARK_CONF_LARGE
from spyt import spark_session
import pyspark
import pyspark.sql.functions as F
import pyspark.sql.types as T
from pyspark.sql.functions import col
from pyspark.sql.session import SparkSession
from pyspark.sql.window import Window


logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


class CollectTrustData:

    balance_history = '//home/cloud_analytics/ml/ba_trust/balance_history'
    consumption_history = '//home/cloud-dwh/data/prod/cdm/dm_yc_consumption'
    currency_rates = '//home/cloud_analytics/ml/ba_trust/currency_rates'
    ba_history = '//home/cloud-dwh/data/prod/ods/billing/billing_accounts_history'
    dm_ba_crm_tags = '//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags'
    billing_accounts = '//home/cloud-dwh/data/prod/ods/billing/billing_accounts'
    dm_events = '//home/cloud-dwh/data/prod/cdm/dm_events'
    result_path = '//home/cloud_analytics/ml/ba_trust/all_data'
    bnpl_score_path = '//home/cloud_analytics/ml/ba_trust/mlp'

    def __init__(self, spark: SparkSession) -> None:
        self.spark = spark

    def test_dates(self) -> None:
        read = self.spark.read.yt
        logger.debug({
            self.dm_ba_crm_tags: read(self.dm_ba_crm_tags).select(F.max(col('date')).alias('date')).collect()[0]['date'],
            self.balance_history: (
                read(self.balance_history)
                .select(F.max(F.to_date(F.from_unixtime(col("created_at").cast(T.LongType())), 'yyyy-MM-dd HH:mm:ss')).cast(T.StringType()).alias('date'))
                .collect()[0]['date']
            ),
            '//home/cloud-dwh/data/prod/ods/billing/transactions': (
                read("//home/cloud-dwh/data/prod/ods/billing/transactions")
                .select(F.max(F.to_date(F.from_unixtime(col("created_at").cast(T.LongType())), 'yyyy-MM-dd HH:mm:ss')).cast(T.StringType()).alias('date'))
                .collect()[0]['date']
            ),
            self.consumption_history: read(self.consumption_history).select(F.max(col('billing_record_msk_date')).alias('date')).collect()[0]['date'],
            self.ba_history: (
                read(self.ba_history)
                .select(F.max(F.to_date(F.from_unixtime(col("updated_at").cast(T.LongType())), 'yyyy-MM-dd HH:mm:ss')).cast(T.StringType()).alias('date'))
                .collect()[0]['date']
            ),
            self.bnpl_score_path: read(self.bnpl_score_path).select(F.max(col('date')).alias('date')).collect()[0]['date'],
            self.currency_rates: read(self.currency_rates).select(F.max(col('msk_date')).alias('date')).collect()[0]['date'],
            self.dm_events: read(self.dm_events).select(F.max(col('msk_event_dt')).alias('date')).collect()[0]['date'],
        })

    def get_all_ba_dates(self) -> pyspark.sql.dataframe.DataFrame:
        """Builds SparkDataframe with ba vs dates grid
        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * billing_account_id
            * msk_date
            * date
            * unix_date
        """
        ba_cross_dates = (
            self.spark.read.yt(self.dm_ba_crm_tags)
            .filter(col('date') >= '2018-01-01')
            .withColumn('unix_date', F.unix_timestamp(F.to_date(col('date'))))
            .select(col('date').alias('msk_date'), col('date').cast(T.StringType()).alias('date'), 'unix_date', 'billing_account_id').distinct()
        )
        last_date = ba_cross_dates.select(F.max('unix_date').alias('unix_date')).collect()[0]['unix_date']

        new_dates = (
            ba_cross_dates
            .filter(col('unix_date')==last_date)
            .select('billing_account_id')
            .crossJoin(
                self.spark.range(last_date, int(time.time()), 86400)
                .select(col('id').alias('unix_date'))
            )
            .withColumn('msk_date', F.to_date(F.from_unixtime(col("unix_date").cast(T.LongType())), 'yyyy-MM-dd HH:mm:ss'))
            .select('msk_date', col('msk_date').cast(T.StringType()).alias('date'), 'unix_date', 'billing_account_id')
        )
        ba_cross_dates = ba_cross_dates.join(new_dates, how='outer', on=['msk_date', 'date', 'unix_date', 'billing_account_id'])
        logger.debug('Loaded BAs ')
        return ba_cross_dates

    def get_payment_history(self) -> pyspark.sql.dataframe.DataFrame:
        """Builds SparkDataframe with billing account payment history
        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * billing_account_id
            * msk_date
            * paid_amount
        """

        billing_history = (
            self.spark.read.yt(self.balance_history)
            .withColumn('msk_date',
                        F.to_date(F.from_unixtime(col("created_at").cast(T.LongType())), 'yyyy-MM-dd HH:mm:ss'))
            .filter(col('status') == 'ok')
            .filter(col('transaction_type') == 'payments')
            .filter(~col('is_aborted'))
            .join(
                self.spark.read.yt(self.currency_rates),
                on=['msk_date', 'currency'],
                how='left'
            )
            .fillna(1, subset=['quote'])
            .withColumn('paid_amount', col('amount')*col('quote'))
            .groupby('billing_account_id', 'msk_date')
            .agg(F.sum('paid_amount').alias('paid_amount'))
            .select('billing_account_id', 'msk_date', 'paid_amount')
        )
        logger.debug('Loaded payment history')
        return billing_history

    def get_consumption_history(self) -> pyspark.sql.dataframe.DataFrame:
        """Builds SparkDataframe with billing account consumption history
        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * billing_account_id
            * msk_date
            * billing_record_cost_rub
            * billing_record_total_rub
        """

        consumption_history = (
            self.spark.read.yt(self.consumption_history)
            .groupby('billing_account_id', 'billing_record_msk_date')
            .agg(
                F.sum('billing_record_cost_rub').alias('billing_record_cost_rub'),
                F.sum('billing_record_total_rub').alias('billing_record_total_rub')
            )
            .select(
                'billing_account_id',
                'billing_record_cost_rub',
                'billing_record_total_rub',
                col('billing_record_msk_date').alias('msk_date')
            )
        )

        logger.debug('Loaded consumption history')
        return consumption_history

    def get_windowed_data(self) -> pyspark.sql.dataframe.DataFrame:
        """Builds SparkDataframe with all date aggregates needed for target construction
        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * billing_account_id
            * date
            * unix_date
            * paid_amount
            * previous_cost
            * debt
            * unchanging_debt_in_30d
            * unchanging_lazy_debt_in_30d
            * wont_pay_anymore_in_30d
        """

        w_all = (Window.partitionBy('billing_account_id').orderBy('msk_date').rangeBetween(Window.unboundedPreceding, Window.currentRow))
        w_all_next = (Window.partitionBy('billing_account_id').orderBy('msk_date').rangeBetween(Window.currentRow, Window.unboundedFollowing))
        # 86400 : number of seconds in one day; we aggregate over 30 days range
        w_next_30d = Window.partitionBy('billing_account_id').orderBy(col('unix_date') / 86400).rangeBetween(1, 30)

        ba_cross_dates = self.get_all_ba_dates()
        billing_history = self.get_payment_history()
        consumption_history = self.get_consumption_history()

        history = (
            ba_cross_dates
            .join(billing_history, on=['billing_account_id', 'msk_date'], how='left')
            .join(consumption_history, on=['billing_account_id', 'msk_date'], how='left')
            .fillna(0, subset=['paid_amount', 'billing_record_cost_rub', 'billing_record_total_rub'])
            .withColumn('previous_cost', F.sum('billing_record_cost_rub').over(w_all))
            .withColumn('previous_total_records', F.sum('billing_record_total_rub').over(w_all))
            .withColumn('previous_payments', F.sum('paid_amount').over(w_all))
            .withColumn('debt', col('previous_total_records') - col('previous_payments'))
            .withColumn('wont_pay_anymore', F.max('paid_amount').over(w_all_next) == 0)
            .withColumn('wont_pay_anymore_in_30d', F.max('wont_pay_anymore').over(w_next_30d))
            .withColumn('unchanging_debt_in_30d', F.last('debt').over(w_next_30d))
            .withColumn('unchanging_lazy_debt_in_30d', F.last('unchanging_debt_in_30d').over(w_all_next))
            .select(
                'paid_amount',
                'billing_account_id',
                'unix_date',
                'previous_cost',
                col('msk_date').cast(T.StringType()).alias('date'),
                'debt',
                'unchanging_debt_in_30d',
                'unchanging_lazy_debt_in_30d',
                'wont_pay_anymore_in_30d'
            )
            .distinct()
        )

        logger.debug('Window collected')
        return history

    def get_time_specific_features(self) -> pyspark.sql.dataframe.DataFrame:
        """Builds SparkDataframe with all time dependent features
        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * billing_account_id
            * date
            * autopay_failures
            * is_isv
            * is_subaccount
            * is_suspended_by_antifraud
            * is_fraud
            * is_var
            * payment_type
            * person_type
            * segment
            * state
            * usage_status
            * last_payment
        """

        autopay = (
            self.spark.read.yt(self.ba_history)
            .select(
                'billing_account_id',
                F.to_date(F.from_unixtime(col('updated_at').cast(T.LongType())), 'yyyy-MM-dd HH:mm:ss').cast(T.StringType()).alias('date'),
                'autopay_failures'
            )
            .distinct()
        )

        bnpl_score = (
            self.spark.read.yt(self.bnpl_score_path)
            .select(
                'billing_account_id',
                'date',
                'score'
            )
            .distinct()
        )

        tags = (
            self.spark.read.yt(self.dm_ba_crm_tags)
            .select(
                col('date').cast(T.StringType()).alias('date'),
                'billing_account_id',
                'is_isv',
                'is_subaccount',
                'is_suspended_by_antifraud',
                'is_fraud',
                'is_var',
                'payment_type',
                'person_type',
                'segment',
                'state',
                'usage_status'
            )
            .distinct()
        )

        w = (Window.partitionBy('billing_account_id').orderBy('msk_date').rangeBetween(Window.unboundedPreceding, Window.currentRow))
        ba_cross_dates = self.get_all_ba_dates()

        last_payment = (
            ba_cross_dates
            .join(
                self.spark.read.yt(self.balance_history)
                .filter(col('status') == 'ok')
                .filter(col('transaction_type') == 'payments')
                .filter(~col('is_aborted'))
                .select(
                    'billing_account_id',
                    F.to_date(F.from_unixtime(col("modified_at").cast(T.LongType())), 'yyyy-MM-dd HH:mm:ss').alias('msk_date'),
                    col("modified_at").alias('payment_date')),
                on=['billing_account_id', 'msk_date'], how='left'
            )
            .withColumn('last_payment', F.max('payment_date').over(w))
            .select('billing_account_id', col('msk_date').cast(T.StringType()).alias('date'), 'last_payment')
            .distinct()
        )

        time_specific = (
            last_payment
            .join(autopay, on=['billing_account_id', 'date'], how='left')
            .join(bnpl_score, on=['billing_account_id', 'date'], how='left')
            .join(tags, on=['billing_account_id', 'date'], how='left')
            .select(col('*')).distinct()
        )

        logger.debug('Loaded time specific')
        return time_specific

    def get_ba_core_features(self) -> pyspark.sql.dataframe.DataFrame:
        """Builds SparkDataframe with all time independent features
        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * billing_account_id
            * created_at
            * became_paid
        """
        created = (
            self.spark.read.yt(self.billing_accounts)
            .select(
                'billing_account_id',
                col('created_at').alias('ba_created_at')
            )
            .distinct()
        )

        became_paid = (
            self.spark.read.yt(self.dm_events)
            .filter(col('event_type') == 'billing_account_became_paid')
            .groupby('billing_account_id')
            .agg(F.min(col('event_timestamp')).alias('became_paid'))
            .select(col('*')).distinct()
        )

        ba_core_features = (
            became_paid
            .join(created, on='billing_account_id', how='outer')
            .select(col('*')).distinct()
        )

        logger.debug('Loaded core features')
        return ba_core_features

    def join_n_aggregate(self) -> pyspark.sql.dataframe.DataFrame:
        """
        Collects final SparkDataframe
        """
        history = self.get_windowed_data()
        time_specific = self.get_time_specific_features()
        ba_core_features = self.get_ba_core_features()
        logger.debug('Preprocessing collected')
        window = Window.partitionBy('billing_account_id').orderBy('date').rowsBetween(Window.unboundedPreceding, 0)

        data_site = (
            history
            .join(time_specific, on=['billing_account_id', 'date'], how='left')
            .join(ba_core_features, on='billing_account_id', how='left')
            .select(col('*')).distinct()
            .withColumn('is_suspended', (col('state') == 'suspended').cast(T.IntegerType()))
            .withColumn('autopay_failures_count', F.last(col('autopay_failures'), ignorenulls=True).over(window))
            .withColumn('days_from_created', ((col('unix_date') - col('ba_created_at')) / 86400).cast(T.IntegerType()))  # 86400 : number of seconds in one day
            .withColumn(
                'days_after_last_payment',
                F.last(((col('unix_date') - col('last_payment')) / 86400).cast(T.IntegerType()), ignorenulls=True).over(window)
            )
            .withColumn(
                'days_after_became_paid',
                F.when(col('unix_date') >= col('became_paid'), ((col('unix_date') - col('became_paid')) / 86400).cast(T.IntegerType())).otherwise(10000)
            )
        )

        logger.debug('Joined')
        # select relevant
        data_filtered = (
            data_site
            .filter(col('usage_status') != 'service')
            .filter(~col('state').isin(['inactive', 'deleted']))
            .filter(col('usage_status') != 'disabled')
            .select(
                'billing_account_id',
                'date',
                'unix_date',
                'autopay_failures_count',
                'paid_amount',
                'previous_cost',
                'unchanging_debt_in_30d',
                'unchanging_lazy_debt_in_30d',
                'wont_pay_anymore_in_30d',
                col('is_isv').cast(T.IntegerType()).alias('is_isv'),
                col('is_subaccount').cast(T.IntegerType()).alias('is_subaccount'),
                col('is_suspended_by_antifraud').cast(T.IntegerType()).alias('is_suspended_by_antifraud'),
                col('is_var').cast(T.IntegerType()).alias('is_var'),
                F.when(col('payment_type') == "invoice", 1).otherwise(0).alias('payment_type'),
                F.when(col('person_type').like('%company'), 1).otherwise(0).alias('person_type'),
                'segment',
                'state',
                col('score').alias('bnpl_score'),
                'usage_status',
                'days_from_created',
                'days_after_last_payment',
                'days_after_became_paid'
            )
            .distinct()
        )

        logger.debug('Collected all features')
        data_filtered.coalesce(1).write.mode("overwrite").yt(self.result_path)
        logger.debug('Uploaded all features')
        return data_filtered


if __name__ == '__main__':
    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_LARGE, driver_memory='2G') as spark:
        data = CollectTrustData(spark)
        data.test_dates()
        data.join_n_aggregate()
