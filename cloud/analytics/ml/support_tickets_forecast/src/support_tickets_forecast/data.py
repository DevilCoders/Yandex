import typing as tp
from datetime import datetime

import numpy as np
import pandas as pd
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from pyspark.sql.session import SparkSession
from pyspark.sql.column import Column as SparkColumn

from clan_tools.data_adapters.YTAdapter import YTAdapter


def linear_fix_ts(dts: pd.Series, dt_from: str, dt_to: str) -> pd.Series:
    dts = dts.copy()

    dt_from, dt_to = pd.to_datetime(dt_from), pd.to_datetime(dt_to)
    dts[(dts.index >= dt_from) & (dts.index <= dt_to)] = np.nan
    dts = dts.interpolate(method='linear', limit_direction='forward')

    return dts


def ifs(cond: SparkColumn, col_if_true: SparkColumn, col_if_false: SparkColumn) -> SparkColumn:
    """Short `F.when(cond, col_if_true).otherwise(col_if_false)` construction

    :param cond: True/False (bool) column
    :param col_if_true: column to take value from when True
    :param col_if_false: column to take value from when False
    :return: result pyspark column
    """
    return F.when(cond, col_if_true).otherwise(col_if_false)


class TicketsDataset:
    """Collect features and target for Support tickets forecast"""

    ticket_line_1_issues = '//home/cloud-dwh/data/prod/cdm/support/dm_yc_support_issues'
    ticket_line_2_issues = '//home/cloud-dwh/data/prod/cdm/support/line_two/dm_yc_support_issues'
    yc_consumption = '//home/cloud_analytics/ml/ml_model_features/by_baid/consumption'
    yc_payments = '//home/cloud_analytics/ml/ml_model_features/by_baid/payments'
    dm_ba_crm_tags = '//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags'

    def __init__(self, spark: SparkSession, yt_adapter: YTAdapter) -> None:
        self._spark = spark
        self._yt_adapter = yt_adapter
        self._today = datetime.now().strftime('%Y-%m-%d')
        self._features: tp.Optional[tp.List[str]] = None

    def get_y(self) -> pd.DataFrame:
        """Collect target"""
        spdf_tickets_line_1 = (
            self._spark.read.yt(self.ticket_line_1_issues)
            .select(
                F.date_format(F.to_timestamp('created_at'), 'yyyy-MM-dd').alias('date'),
                'billing_account_id',
                'issue_id',
                'components_quotas'
            )
        )

        spdf_tickets_line_2 = (
            self._spark.read.yt(self.ticket_line_2_issues)
            .select(
                F.date_format(F.to_timestamp('created_at'), 'yyyy-MM-dd').alias('date'),
                'billing_account_id',
                'issue_id',
                'components_quotas'
            )
        )

        spdf_segment = self._spark.read.yt(self.dm_ba_crm_tags).select('date', 'billing_account_id', 'segment')

        segment_list = ['Mass', 'Medium', 'Enterprise', 'Public sector']
        spdf_tickets =(
            spdf_tickets_line_1.union(spdf_tickets_line_2)
            .join(spdf_segment, on=['date', 'billing_account_id'], how='left')
            .filter(col('date') < datetime.now().strftime('%Y-%m-%d'))
            .filter(col('date') >= '2019-01-01')
            .withColumn('segment', ifs(col('segment').isin(segment_list), col('segment'), lit('Other')))
            .distinct()
            .groupby('date', 'segment', 'components_quotas')
            .agg(F.count('issue_id').alias('count'))
            .sort('date')
            .cache()
        )

        df_tickets = spdf_tickets.toPandas()
        df_tickets['date'] = pd.to_datetime(df_tickets['date'])
        df_tickets = df_tickets.set_index('date')

        return df_tickets

    def _collect_features(self) -> pd.DataFrame:
        """Collect features"""
        # consumption
        yc_cons_tables = sorted([f'{self.yc_consumption}/{table}' for table in self._yt_adapter.yt.list(self.yc_consumption)])
        spdf_consumption = (
            self._spark.read.yt(*yc_cons_tables)
            .filter(col('date') < self._today)
            .groupby('date')
            .agg(
                F.count('billing_account_id').alias('total_accs'),
                F.sum(ifs(col('billing_record_cost_rub') > 1, lit(1), lit(0))).alias('total_active_accs'),
                F.sum(ifs(col('billing_record_total_rub') > 1, lit(1), lit(0))).alias('total_paid_accs'),
                F.round(F.sum('billing_record_cost_rub'), 2).alias('total_cost'),
                F.round(-F.sum('billing_record_credit_rub'), 2).alias('total_grants'),
                F.round(F.sum('sku_non_lazy_total_rub'), 2).alias('total_non_lazy')
            )
            .withColumn('mean_cost', col('total_cost')/col('total_active_accs'))
            .sort('date')
            .cache()
        )

        # transactions
        yc_paym_tables = [f'{self.yc_payments}/{table}' for table in self._yt_adapter.yt.list(self.yc_payments)]
        spdf_transacts = (
            self._spark.read.yt(*yc_paym_tables)
            .filter(col('date') < self._today)
            .groupby('date')
            .agg(
                F.sum(ifs((col('days_from_created') == 7) & (col('state') == 'active'), lit(1), lit(0))).alias('new_ba_created_7d'),
                F.sum(ifs(col('person_type').rlike('individual') & (col('state') == 'active'), lit(1), lit(0))).alias('ba_individual'),
                F.sum(ifs(col('person_type').rlike('company') & (col('state') == 'active'), lit(1), lit(0))).alias('ba_company'),
                F.coalesce(F.sum(ifs(col('paid_amount') > 1e-3, col('paid_amount'), lit(0))), lit(0)).alias('ba_rec_transacted'),
            )
            .sort('date')
            .cache()
        )

        spdf = (
            spdf_consumption
            .join(spdf_transacts, on='date', how='outer')
            .sort('date')
        )

        df_features = spdf.toPandas()
        df_features['date'] = pd.to_datetime(df_features['date'])
        df_features = df_features.set_index('date').resample('1D').mean().ffill()

        return df_features

    def get_X_y(self) -> tp.Tuple[pd.DataFrame, pd.Series]:
        """Create X, y dataset"""
        df_target = self.get_y()
        df_target = df_target[df_target['components_quotas'] == False]  # noqa: E712
        df_target = df_target['count'].resample('1D').sum().fillna(0)
        df_features = self._collect_features()

        date_from = max(df_features.index.min(), df_target.index.min())
        date_to = min(df_features.index.max(), df_target.index.max())

        X = df_features[(df_features.index >= date_from) & (df_features.index <= date_to)].copy()
        y = df_target[(df_target.index >= date_from) & (df_target.index <= date_to)].copy()
        self._features = X.columns.tolist()

        assert not X.isna().any().any()
        assert not y.isna().any()

        return X, y

    def get_fetures_names(self) -> tp.List[str]:
        if self._features is None:
            raise AttributeError('Not loaded yet. It gives features names after running method `get_X_y`')
        else:
            return self._features


__all__ = ['TicketsDataset', 'linear_fix_ts']
