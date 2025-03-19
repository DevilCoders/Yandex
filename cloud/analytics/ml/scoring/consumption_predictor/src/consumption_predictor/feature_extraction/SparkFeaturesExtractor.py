import logging
from datetime import datetime, timedelta
from itertools import groupby

import numpy as np
import pandas as pd
import pyspark.sql.functions as F
import pyspark.sql.types as T
from pyspark.ml.feature import Word2Vec
from pyspark.sql.functions import broadcast, col, countDistinct, lit
from pyspark.sql.window import Window
import pyspark.sql.dataframe as spd

from consumption_predictor.data_adapters.SparkFeaturesAdapter import \
    SparkFeaturesAdapter

from clan_tools.utils.spark import prepare_for_yt

logger = logging.getLogger(__file__)


class SparkFeaturesExtractor:
    def __init__(self, features_adapter: SparkFeaturesAdapter, target_cons=30000,
                 hist_period=14,  pred_period=1, paid_cons_started=200):
        self._features_adapter : SparkFeaturesAdapter = features_adapter
        self._target_cons = target_cons
        self._hist_period = hist_period
        self._pred_period = pred_period
        self._paid_cons_started = paid_cons_started
        self._day_use_df = None
        self._email_domains = None

    @property
    def _day_use(self):
        if self._day_use_df is None:
            day_use = self._features_adapter.get_day_use()
            vm_df = self._features_adapter.get_vm_data()
            day_use_df = day_use.join(
                vm_df, on=['billing_account_id', 'event_time'], how='left')
            self._day_use_df = day_use_df
        return self._day_use_df

    def _group_by(self, pred=False):
        return ['billing_account_id'] if pred else ['billing_account_id', 'week']

    def _cons_by_services(self, acc_df: spd.DataFrame, pred=False):
        return acc_df.groupBy(self._group_by(pred))\
            .pivot('subservice_name') \
            .sum('real_consumption').na.fill(0)

    def _add_vm_features(self, acc_df: spd.DataFrame, weekly_cons_future: spd.DataFrame, pred=False):
        vm_features = acc_df.groupby(self._group_by(pred))\
            .max('vm_count', 'sum_cores')
        weekly_cons_future = weekly_cons_future.join(
            vm_features, on=self._group_by(pred))
        return weekly_cons_future.na.fill(0)

    def _add_ts_features(self, acc_df: spd.DataFrame, weekly_cons_future: spd.DataFrame, pred=False):
        def _slope(ts):
            res = 0.
            if len(ts) > 3:
                res = np.nanmean(ts[len(ts)//2:]) - np.nanmean(ts[:len(ts)//2])
            return float(res)
        slope_udf = F.udf(_slope, T.DoubleType())
        ts_features = acc_df.groupby(self._group_by(pred))\
            .agg(F.mean('real_consumption').alias('rc_mean'),
                 F.stddev('real_consumption').alias('rc_std'),
                 slope_udf(F.collect_list('real_consumption')).alias('slope')
                 )

        weekly_cons_future = weekly_cons_future.join(
            ts_features, on=self._group_by(pred))
        return weekly_cons_future

    def _add_total_consumption(self, acc_df: spd.DataFrame,  weekly_cons_future: spd.DataFrame, pred=False):
        consumptions = acc_df.fillna(0).groupby(self._group_by(pred))\
            .agg({'real_consumption': 'sum',  
                  'balance': 'max',
                  'br_cost': 'sum',
                  'br_var_reward_rub': 'sum',
                  'br_credit_grant_rub': 'sum',
                  'br_credit_service_rub': 'sum',
                  'br_credit_cud_rub': 'sum',
                  'br_credit_volume_incentive_rub': 'sum',
                  'br_credit_disabled_rub': 'sum',
                  'br_credit_trial_rub': 'sum',
                  })
        weekly_cons_future = weekly_cons_future.join(
            consumptions, on=self._group_by(pred)).na.fill(0)
        return weekly_cons_future

    def _add_phone(self, acc_df: spd.DataFrame,  weekly_cons_future: spd.DataFrame, pred=False):
        phone = acc_df.groupby(self._group_by(pred)).agg(
            F.first('phone').alias('phone'))
        weekly_cons_future = weekly_cons_future.join(
            phone, on=self._group_by(pred))
        return weekly_cons_future

    def _add_email(self, acc_df: spd.DataFrame,  weekly_cons_future: spd.DataFrame, pred=False):
        get_email_domain = F.split(col('email'), '@').getItem(1).alias('email_domain')
        

        if not pred:
            email_domains = (
                acc_df
                .select('billing_account_id', get_email_domain)
                .groupby('email_domain')
                .agg(F.countDistinct('billing_account_id').alias('accs_with_email_domain'))
            ).cache()
            self._email_domains = email_domains

        accs_email_domains = (
            acc_df
            .groupby(self._group_by(pred))
            .agg(F.first('email').alias('email'))
            .withColumn('email_domain', get_email_domain)
            .join(self._email_domains, on='email_domain')
        )

        weekly_cons_future = weekly_cons_future.join(
            accs_email_domains, on=self._group_by(pred))
        return weekly_cons_future

    def _add_utm_source(self, acc_df: spd.DataFrame,  weekly_cons_future: spd.DataFrame, pred=False):
        utm_source = acc_df.withColumn('is_direct', col('utm_source') == 'direct')\
            .groupby(self._group_by(pred)).agg(F.first('is_direct').alias('is_direct'))
        weekly_cons_future = weekly_cons_future.join(
            utm_source, on=self._group_by(pred))
        return weekly_cons_future

    def _add_company(self, acc_df: spd.DataFrame,  weekly_cons_future: spd.DataFrame, pred=False):
        company = acc_df.withColumn('company', col('ba_person_type') == 'company')\
            .groupby(self._group_by(pred)).agg(F.max('company').alias('company'))
        weekly_cons_future = weekly_cons_future.join(
            company, on=self._group_by(pred))
        return weekly_cons_future

    def _add_week_from_first_consumption(self, day_use_df: spd.DataFrame):
        acc_df = day_use_df[day_use_df['real_consumption'] > 0]
        first_use = day_use_df[day_use_df['real_consumption']
                               > self._paid_cons_started]
        first_use = first_use.groupBy('billing_account_id')\
            .agg(F.min('event_time').alias('first_use'))\
            .select(['billing_account_id', 'first_use'])
        acc_df = acc_df.join(first_use, on='billing_account_id', how='inner')\
            .withColumn('days_since_start', F.datediff('event_time', 'first_use')) \
            .withColumn('week', (col('days_since_start') / self._hist_period).cast('int'))
        return acc_df.cache(), first_use.cache()

    def _reindex_weeks(self, acc_df: spd.DataFrame, first_use, weekly_cons_future: spd.DataFrame):
        all_weeks = acc_df.groupby('billing_account_id')\
            .agg(F.max('week').alias('max_week'))\
            .withColumn('weeks', F.sequence(lit(0), col('max_week')))\
            .withColumn('week', F.explode('weeks'))\
            .select(['billing_account_id', 'week'])
        weekly_cons_future = all_weeks.join(
            weekly_cons_future, on=['billing_account_id', 'week'], how='left')
        weekly_cons_future = weekly_cons_future.join(first_use, on=['billing_account_id'])\
            .withColumn('start_week', F.expr(f"date_add(to_date(first_use), {self._hist_period}*week)"))\
            .withColumn('end_week', F.expr(f"date_add(to_date(first_use), {self._hist_period}*week + {self._hist_period})"))
        return weekly_cons_future.fillna(0).cache()

    def _add_next_week_cons_as_target(self, weekly_cons_future: spd.DataFrame):
        weekly_cons_future = weekly_cons_future\
            .withColumn('target', F.lag('sum(real_consumption)', offset=-1)
                        .over(Window.orderBy('week').partitionBy('billing_account_id')))\
            .fillna(0)
        return weekly_cons_future

    def _add_net_logs(self, weekly_cons_future: spd.DataFrame, date_from=None, pred=False, vec_size=10):
        word2Vec = Word2Vec(vectorSize=vec_size, minCount=0, inputCol="url_hist", numPartitions=16,
                            outputCol="w2v", maxIter=1)
        net = self._features_adapter.get_net_logs()
        if not pred:
            net_hist = weekly_cons_future.join(net, on=((weekly_cons_future.billing_account_id == net.ba)
                                                        & net.timestamp.between(weekly_cons_future.start_week,
                                                                                weekly_cons_future.end_week)))
        else:
            net_hist = weekly_cons_future.join(net, on=((weekly_cons_future.billing_account_id == net.ba)
                                                        & (net.timestamp > date_from)))
        net_hist = net_hist.groupBy(self._group_by(pred))\
            .agg(F.collect_list('event_url').alias('url_hist'))
        model = word2Vec.fit(net_hist)
        net_hist = model.transform(net_hist)
        weekly_cons_future = weekly_cons_future\
            .join(net_hist, on=self._group_by(pred), how='left')\
            .withColumn("w2v", self._to_array(col("w2v"))) 
        weekly_cons_future = weekly_cons_future\
            .select([cols for cols in weekly_cons_future.columns if cols not in ('w2v', 'url_hist')] 
            + [col("w2v")[i].alias(f'w2v_{i}') for i in range(vec_size)])
        return weekly_cons_future.na.fill(0)

    def _add_account_name(self, acc_df: spd.DataFrame,  weekly_cons_future: spd.DataFrame, pred=False):
        acc_names = acc_df.groupby(self._group_by(pred)).agg(
            F.max('account_name').alias('account_name'))
        weekly_cons_future = weekly_cons_future.join(
            acc_names, on=self._group_by(pred))
        return weekly_cons_future

    def _count_same_phone(self, acc_df: spd.DataFrame,  weekly_cons_future: spd.DataFrame, pred=False):
        groupby_cols = 'phone' if pred else ['phone', 'week']
        same = acc_df.groupby(groupby_cols).agg(
            F.countDistinct('billing_account_id').alias('same_phone'))
        weekly_cons_future = weekly_cons_future.join(same, on=groupby_cols)
        return weekly_cons_future

    def _first_target_features_by_ba(self, features: pd.DataFrame):
        features_first_target = features[features['target'] >= self._target_cons].\
            select(col('*'), F.row_number()
                   .over(Window.partitionBy('billing_account_id').orderBy('week'))
                   .alias('row_number')) \
            .where(col('row_number') == 1)\
            .select(col('billing_account_id'), col('week').alias('first_target_week'))
        return features_first_target

    def _features_before_target(self, features: spd.DataFrame, features_first_target: spd.DataFrame):
        features = features.join(
            features_first_target, on='billing_account_id', how='left')
        features = features[((features.week <= features.first_target_week)
                             & col('first_target_week').isNotNull())
                            | col('first_target_week').isNull()]
        return features

    def _to_array(self, col):
        def to_array_(v):
            if v is not None:
                return v.toArray().tolist()
            else:
                return [0]*10
        return F.udf(to_array_, T.ArrayType(T.DoubleType()))(col)


    def _add_leads_labels(self, features: spd.DataFrame):
        leads_labels = self._features_adapter.get_crm_leads()
        features = (features
            .join(leads_labels, on='billing_account_id', how='left')
            .withColumn('label',
                F.when(col('lead_label').isNotNull(), 
                        col('lead_label')).otherwise(col('label')))).drop('lead_label')
        
        return features

    # @cached('./data/cache/get_features.pkl') #TODO CACHE
    @prepare_for_yt
    def get_features(self):
        logger.info('Started to generate features')
        day_use_df = self._day_use
        acc_df, first_use = self._add_week_from_first_consumption(day_use_df)
        weekly_cons_future = self._cons_by_services(acc_df)
        # logger.debug(f'_cons_by_services {(weekly_cons_future.count(), len(weekly_cons_future.columns))}')

        weekly_cons_future = self._add_total_consumption(
            acc_df, weekly_cons_future)

        # logger.debug(f'total cons {(weekly_cons_future.count(), len(weekly_cons_future.columns))}')

        weekly_cons_future = self._add_vm_features(acc_df, weekly_cons_future)
        # logger.debug(f'_vm_features {(weekly_cons_future.count(), len(weekly_cons_future.columns))}')
        weekly_cons_future = self._add_ts_features(acc_df, weekly_cons_future)
        # logger.debug(f'_ts_features {(weekly_cons_future.count(), len(weekly_cons_future.columns))}')
        weekly_cons_future = self._reindex_weeks(
            acc_df, first_use, weekly_cons_future)

        # logger.debug(f'_reindex weeks {(weekly_cons_future.count(), len(weekly_cons_future.columns))}')
        # weekly_cons_future = self._add_net_logs(weekly_cons_future)

        weekly_cons_future = self._add_phone(acc_df, weekly_cons_future)
        weekly_cons_future = self._add_email(acc_df, weekly_cons_future)

        weekly_cons_future = self._add_utm_source(acc_df, weekly_cons_future)
        weekly_cons_future = self._add_company(acc_df, weekly_cons_future)
        weekly_cons_future = self._add_account_name(acc_df, weekly_cons_future)
        weekly_cons_future = self._count_same_phone(acc_df, weekly_cons_future)

        features = self._add_next_week_cons_as_target(weekly_cons_future).cache()
        # logger.debug(f'_add_next_week_cons_as_target {(features.count(), len(features.columns))}')
        features = features.withColumn('target', col(
            'sum(real_consumption)') + col('target'))

        features = features[(features['sum(real_consumption)'] > 0) &
                            (features['sum(real_consumption)'] < self._target_cons)]
        # logger.debug(f'no targets in train {(features.count(), len(features.columns))}')

        features_first_target = self._first_target_features_by_ba(features)
        # logger.debug(f'features_first_target{(features_first_target.count(), len(features_first_target.columns))}')

        features = self._features_before_target(
            features, features_first_target)
        # logger.debug(f'features before target {(features.count(), len(features.columns))}')

        features = features.withColumn(
            'label', (features['target'] >= self._target_cons).cast("integer")).fillna(0)

        target_bas = features.groupby('billing_account_id').agg(F.max('label').alias('label'))
        features = features.drop('label').join(target_bas, on='billing_account_id')

        features = self._add_leads_labels(features)

        return features


    @prepare_for_yt
    def pred_features(self,  features: spd.DataFrame):

        ba_is_target = features.groupby('billing_account_id').agg(
            F.sum('label').alias('label'), F.max('week').alias('week'))\
            .withColumn('week', col('week') + 1)

        day_use_df = self._day_use
        logger.info(
            f'Collecting dataset for prediction for last {self._hist_period}')
        last_week_start = datetime.now() - timedelta(days=self._hist_period)
        last_week_use_df = day_use_df[(day_use_df['real_consumption'] > 0)
                                      & (day_use_df['sales_name'] == 'unmanaged')
                                      & (day_use_df['segment'] == 'Mass')
                                      & (day_use_df['event_time'] > last_week_start)].cache()

        weekly_cons_future = self._cons_by_services(
            last_week_use_df, pred=True)
        weekly_cons_future = self._add_vm_features(
            last_week_use_df, weekly_cons_future, pred=True)
        weekly_cons_future = self._add_ts_features(
            last_week_use_df, weekly_cons_future, pred=True)
        weekly_cons_future = self._add_total_consumption(
            last_week_use_df, weekly_cons_future, pred=True)
        # weekly_cons_future = self._add_net_logs(
        #     weekly_cons_future, date_from=last_week_start, pred=True)
        weekly_cons_future = self._add_phone(
            last_week_use_df, weekly_cons_future, pred=True)
        weekly_cons_future = self._add_email(
            last_week_use_df, weekly_cons_future, pred=True)

        weekly_cons_future = self._add_utm_source(
            last_week_use_df, weekly_cons_future, pred=True)
        weekly_cons_future = self._add_company(
            last_week_use_df, weekly_cons_future, pred=True)
        weekly_cons_future = self._add_account_name(
            last_week_use_df, weekly_cons_future, pred=True)
        weekly_cons_future = self._count_same_phone(
            last_week_use_df, weekly_cons_future, pred=True)

        weekly_cons_future = weekly_cons_future.join(
            ba_is_target, on='billing_account_id', how='left')
        weekly_cons_future = weekly_cons_future[(weekly_cons_future.label.isNull() | (weekly_cons_future['label'] == 0)
                                                 & (weekly_cons_future['sum(real_consumption)'] > self._paid_cons_started))
                                                ]
        return weekly_cons_future.fillna(0)

  
    
