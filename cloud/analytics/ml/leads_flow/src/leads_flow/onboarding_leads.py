import numpy as np
import pandas as pd
import pyspark.sql.functions as F
import pyspark.sql.types as T
import logging.config
import typing as tp
from clan_tools.logging.logger import default_log_config
from datetime import datetime, timedelta
from pyspark.sql.window import Window
from pyspark.sql.functions import col, lit
from pyspark.sql.session import SparkSession
from pyspark.sql.column import Column as SparkColumn
from pyspark.sql.dataframe import DataFrame as SparkDataFrame

from clan_tools.data_adapters.crm.CRMHistoricalDataAdapter import CRMHistoricalDataAdapter


logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def max_by(colname_val: str, colname_by: str) -> SparkColumn:
    return F.expr(f'Max_by(`{colname_val}`, `{colname_by}`)')


def min_by(colname_val: str, colname_by: str) -> SparkColumn:
    return F.expr(f'Min_by(`{colname_val}`, `{colname_by}`)')


class LeadsFlow:
    """ Used for tracking new billing accounts way throug CRM
    """
    path_ba_crm_tags = '//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags'
    path_actual_features = '//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_features'

    def __init__(self, spark: SparkSession, yt_adapter, from_date: tp.Optional[str] = None, to_print: bool = False):
        self.spark = spark
        self.yt_adapter = yt_adapter
        self.today = datetime.now().strftime('%Y-%m-%d')
        self.from_date = from_date or '2022-01-01'  # we decided to start tracking that leads from this date
        self.to_print = to_print

    def _get_base_info(self) -> SparkDataFrame:
        spdf_base = (
            self.spark.read.yt(self.path_ba_crm_tags)
            .groupby('billing_account_id')
            .agg(
                F.min('date').alias('date_created'),
                min_by('segment', 'date').alias('segment_start'),
                max_by('segment_current', 'date').alias('segment_end'),
                max_by('person_type_current', 'date').alias('raw_person_type'),
                max_by('state_current', 'date').alias('state'),
                max_by('usage_status_current', 'date').alias('status'),
                max_by('is_var_current', 'date').astype('int').alias('is_var'),
                max_by('is_isv_current', 'date').astype('int').alias('is_isv'),
                max_by('is_suspended_by_antifraud_current', 'date').astype('int').alias('is_fraud')
            )
            .select(
                'billing_account_id',
                'date_created',
                'segment_start',
                'segment_end',
                self._person_type('raw_person_type').alias('person_type'),
                self._resident('raw_person_type').alias('resident_type'),
                self._state('state').alias('state'),
                'status',
                'is_var',
                'is_isv',
                'is_fraud'
            )
            .filter(col('segment_start')=='Mass')
            .filter(col('person_type')=='company')
            .filter(col('date_created') >= self.from_date)
            .filter(col('date_created') < self.today)
            .cache()
        )

        return spdf_base

    def _person_type(self, colname: str) -> SparkColumn:
        company = F.when(col(colname).rlike('company'), lit('company'))
        individual = F.when(col(colname).rlike('individual'), lit('individual'))
        internal = F.when(col(colname).rlike('internal'), lit('internal'))
        return F.coalesce(company, individual, internal, lit('undefined'))

    def _resident(self, colname: str) -> SparkColumn:
        undefined = F.when(col(colname).isNull(), lit('undefined'))
        fuw = F.split(col(colname), '_')[0]
        is_resident = F.when(fuw.isin(['company', 'individual', 'internal']), lit('resident')).otherwise(fuw)
        return F.coalesce(undefined, is_resident)

    def _state(self, colname: str) -> SparkColumn:
        p_req = ['payment_not_confirmed', 'payment_required']
        p_del = ['deleted', 'inactive']
        res_col = F.when(col(colname).isin(p_req), lit('payment_required')).otherwise(
            F.when(col(colname).isin(p_del), lit('inactive')).otherwise(col(colname)))
        return res_col

    def _get_reasons(self) -> SparkDataFrame:
        spdf_reasons = (
            self.spark.read.yt(self.path_actual_features)
            .filter(col('billing_record_msk_date') >= self.from_date)
            .filter(col('days_from_created') == 7)
            .select(
                'billing_account_id',
                'billing_record_msk_date',
                'billing_record_cost_rub',
                'billing_record_credit_rub',
                'billing_record_total_rub',
                'days_from_created',
                'billing_account_usage_status',
                'billing_account_person_type',
                'billing_account_currency',
                'billing_account_state',
                'billing_account_is_fraud',
                'billing_account_is_suspended_by_antifraud',
                'billing_account_is_isv',
                'billing_account_is_var',
                'billing_account_is_crm_account',
                'crm_partner_manager',
                'crm_segment',
                'prev_7d_cons'
            )
            .cache()
        )

        return spdf_reasons

    def _get_crm_data(self) -> SparkDataFrame:
        historical_data_adapter = CRMHistoricalDataAdapter(self.yt_adapter, self.spark)

        def make_crm_event(ls_name: str, lsd_name: str):
            # initialize columns
            ls = F.coalesce(col(ls_name), lit('unknown'))
            lsd = F.coalesce(col(lsd_name), lit('unknown'))

            ls = F.when(lsd == 'new4upsell', lit('upsell')).otherwise(
                F.when(ls.isin(['upsell', 'trial']), ls).otherwise(lit('other')))

            onb_list = ['Client is Company', 'Client is Individual', 'TrialCompanies',
                        'TrialCompaniesNonresidents', 'NoTrialCompaniesWithBA',
                        'Client is Kazakhstan_individual']
            upsell_list = ['advanced onboarding', 'contact more then 70 days', 'upsell', 'new4upsell']
            lsd = F.when(ls == 'other', lit('Other')).otherwise(
                F.when(ls == 'mkt', lit('Website request')).otherwise(
                    F.when((ls == 'trial') & (lsd.isin(onb_list)), lit('Onboarding')).otherwise(
                        F.when(ls == 'trial', lit('Other')).otherwise(
                            F.when((ls == 'upsell') & lsd.isin(upsell_list), lit('Upsell')).otherwise(
                                F.when(F.lower(lsd).rlike('potential candidate'), lit('CSM')).otherwise(
                                    F.when(F.lower(lsd).rlike('consumed more'), lit('CSM')).otherwise(
                                        lit('Other'))))))))

            return F.concat(ls, lit(' - '), lsd)

        spdf_crm = (
            historical_data_adapter.
            historical_preds()
            .filter(col('billing_account_id').isNotNull())
            .select(
                'billing_account_id',
                F.to_date(F.to_timestamp(col('date_entered').astype(T.LongType())/1e6)).alias('event_date'),
                col('lead_source_crm').alias('lead_source'),
                col('lead_source').alias('lead_source_description'),
                make_crm_event('lead_source_crm', 'lead_source').alias('event')
            )
            .distinct()
            .cache()
        )

        return spdf_crm

    def get_sankey_start(self) -> SparkDataFrame:
        sankey_start = (
            self._get_base_info()
            .select(
                'billing_account_id',
                col('date_created').alias('event_date'),
                lit('BA created').alias('event')
            )
        )

        return sankey_start

    def get_sankey_on_7_days(self) -> SparkDataFrame:
        filtered_crm = (
            self._get_crm_data()
            .filter(col('event').isin(['trial - Onboarding', 'mkt - Website request']))
        )

        df_7_days = (
            self._get_base_info()
            .join(self._get_reasons(), on='billing_account_id', how='left')
            .join(filtered_crm, on='billing_account_id', how='left')
            .toPandas()
        )

        curr_date = datetime.now().replace(hour=0, minute=0, second=0, microsecond=0)
        df_7_days['curr_age'] = (curr_date - pd.to_datetime(df_7_days['date_created'])).dt.days

        # aggregated events
        bind_all = df_7_days['billing_account_id'].notna()
        events_cols = pd.Series(['']*sum(bind_all), index=df_7_days.index, name='event')
        bind_other = bind_all

        bind_leads = df_7_days['event'] == 'trial - Onboarding'
        events_cols.loc[bind_leads] = 'trial - Onboarding'
        bind_other &= ~bind_leads

        bind_leads_mkt = df_7_days['event'] == 'mkt - Website request'
        events_cols.loc[bind_leads_mkt] = 'mkt - Website request'
        bind_other &= ~bind_leads_mkt

        bind_less = bind_other & (df_7_days['curr_age'] <= 7)
        events_cols.loc[bind_less] = 'lifetime < 7 days'
        bind_other &= ~bind_less

        bind_frod = bind_other & df_7_days['is_fraud']
        events_cols.loc[bind_frod] = 'fraud billing-account'
        bind_other &= ~bind_frod

        bind_paid = bind_other & ((df_7_days['prev_7d_cons']>0.) | (df_7_days['billing_account_usage_status']=='paid'))
        events_cols.loc[bind_paid] = 'paid billing-account'
        bind_other &= ~bind_paid

        bind_isv_var = bind_other & (df_7_days['billing_account_is_var'] | df_7_days['billing_account_is_isv'])
        events_cols.loc[bind_isv_var] = 'ba is isv/var'
        bind_other &= ~bind_isv_var

        bind_acc_owner = bind_other & df_7_days['billing_account_is_crm_account']
        events_cols.loc[bind_acc_owner] = 'ba has account-owner'
        bind_other &= ~bind_acc_owner

        bind_state = bind_other & (df_7_days['billing_account_state'] != 'active')
        events_cols.loc[bind_state] = 'ba is not active'
        bind_other &= ~bind_state

        events_cols.loc[bind_other] = 'unknown reason'
        if self.to_print:
            logger.info('TOTAL: %6.d' % sum(bind_all))
            logger.info('='*30)
            logger.info('\t> Leads:     %6.d' % sum(bind_leads))
            logger.info('\t> Leads mkt: %6.d' % sum(bind_leads_mkt))
            logger.info('\t> Age < 7d:  %6.d' % sum(bind_less))
            logger.info('\t> Fraud:     %6.d' % sum(bind_frod))
            logger.info('\t> Paid:      %6.d' % sum(bind_paid))
            logger.info('\t> Isv/Var:   %6.d' % sum(bind_isv_var))
            logger.info('\t> Acc.owner: %6.d' % sum(bind_acc_owner))
            logger.info('\t> State:     %6.d' % sum(bind_state))
            logger.info('\t> Other:     %6.d' % sum(bind_other))

        tdf_7_days = df_7_days[['billing_account_id']].copy()
        tdf_7_days['event_date'] = np.minimum((pd.to_datetime(df_7_days['date_created']) + timedelta(days=7)).dt.strftime('%Y-%m-%d'), self.today)
        tdf_7_days['event'] = events_cols
        self.to_print = False
        sankey_7_days = self.spark.createDataFrame(tdf_7_days)

        return sankey_7_days

    def get_sankey_after_7_days(self) -> SparkDataFrame:
        go_on_events = ['paid billing-account', 'trial - Onboarding', 'ba has account-owner',
                        'ba is isv/var', 'mkt - Website request', 'unknown reason']
        events_filter = (
            self.get_sankey_on_7_days()
            .filter(col('event').isin(go_on_events))
            .select('billing_account_id', col('event_date').alias('date_from'))
        )

        sankey_events = (
            self._get_crm_data()
            .filter(col('event').isin(['upsell - CSM', 'upsell - Upsell']))
            .join(events_filter, on='billing_account_id', how='inner')
            .filter(col('event_date') > col('date_from'))
            .filter(col('event_date') < self.today)
            .groupby('billing_account_id', 'event')
            .agg(F.min('event_date').alias('event_date'))
            .select('billing_account_id', 'event_date', 'event')
        )

        sankey_finish = (
            self._get_base_info()
            .join(events_filter, on='billing_account_id', how='inner')
            .filter(col('date_from') < self.today)
            .select(
                'billing_account_id',
                lit((datetime.now()+timedelta(days=1)).strftime('%Y-%m-%d')).alias('event_date'),
                F.coalesce(
                    F.when(col('state') != 'active', lit('ba is not active ')),
                    F.when(col('is_fraud') == 1, lit('fraud billing-account ')),
                    F.when(col('status') == 'service', lit('service biling_account ')),
                    F.when(col('segment_end') != 'Mass', lit('other segment ')),
                    F.when(col('state') == 'active', lit('active billing-account '))
                ).alias('event')
            )
            .select('billing_account_id', 'event_date', 'event')
        )

        sankey_after_7_days = sankey_events.union(sankey_finish)

        return sankey_after_7_days

    def get_sankey_graph(self) -> SparkDataFrame:
        win = Window.partitionBy('billing_account_id').orderBy(col('event_date').asc(), col('event'))

        sankey_res = (
            self.get_sankey_start()
            .union(self.get_sankey_on_7_days())
            .union(self.get_sankey_after_7_days())
            .select(
                'billing_account_id',
                'event_date',
                F.lag('event').over(win).alias('from'),
                col('event').alias('to'),
                (F.row_number().over(win) - 1).alias('step')
            )
            .filter(col('from').isNotNull())
            .filter(col('from') != col('to'))
        )

        spdf_graph_sankey = (
            sankey_res
            .join(self._get_base_info(), on='billing_account_id', how='left')
            .select(
                lit(self.today).alias('calc_date'),
                'billing_account_id',
                'event_date',
                'from',
                'to',
                'step',
                'date_created',
                col('segment_end').alias('segment'),
                'person_type',
                'resident_type',
                'state',
                'status',
                'is_var',
                'is_isv',
                'is_fraud'
            )
        )

        return spdf_graph_sankey

__all__ = ['LeadsFlow']
