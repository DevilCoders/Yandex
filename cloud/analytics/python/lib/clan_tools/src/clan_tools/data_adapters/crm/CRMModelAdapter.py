import typing as tp
import logging
import pyspark.sql.functions as F
import pyspark.sql.types as T
from pyspark.sql.functions import col, lit
from pyspark.sql.session import SparkSession
from pyspark.sql.column import Column as SparkColumn
from pyspark.sql.dataframe import DataFrame as SparkDataFrame

from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.crm.CRMHistoricalDataAdapter import CRMHistoricalDataAdapter

logger = logging.getLogger(__name__)


class CRMModelAdapter:

    def __init__(self, yt_adapter: YTAdapter, spark: SparkSession, predictions: SparkDataFrame,
                 lead_source: str, only_lead_source: bool = False, leads_daily_limit: int = 3) -> None:
        self._yt_adapter = yt_adapter
        self._spark = spark
        self._historical_data_adapter = CRMHistoricalDataAdapter(yt_adapter, spark)
        self._predictions = predictions
        self._lead_source = lead_source
        self._leads_daily_limit = leads_daily_limit
        self._only_lead_source = only_lead_source

    def _ba_created_info(self) -> SparkDataFrame:
        return (
            self._spark.read.yt('//home/cloud_analytics/cubes/acquisition_cube/cube')
            .filter(col('event') == 'ba_created')
            .filter(col('ba_usage_status') != 'service')
            .filter(col('ba_state') == 'active')
        )

    def _cloud_owners_history(self) -> SparkDataFrame:
        return (
            self._spark.read.yt("//home/cloud_analytics/import/iam/cloud_owners_history")
            .filter(col('passport_uid') != '')
            .filter(col('cloud_status') == 'ACTIVE')
            .select(col('passport_uid').alias('puid'), col('timezone'))
            .drop_duplicates()
        )

    def _prepared_predictions(self) -> SparkDataFrame:
        logger.info('Prepairing predictions to save to CRM')
        acq = self._ba_created_info()
        cloud_owners_history = self._cloud_owners_history().cache()
        preds = self._predictions

        def replace_with_null(colname: str) -> SparkColumn:
            null_val = lit(None).cast(T.StringType())
            is_empty = F.isnull(col(colname)) | (col(colname) == '')
            return F.when(is_empty, null_val).otherwise(col(colname)).alias(colname)

        def replace_with_col(colname: str, repl_col: tp.Union[str, SparkColumn]) -> SparkColumn:
            is_empty = F.isnull(col(colname)) | (col(colname) == '')
            return F.when(is_empty, repl_col).otherwise(col(colname)).alias(colname)

        def _normalize_email() -> SparkColumn:
            is_yandex_mail = col('user_settings_email').like('%@yandex.%') |\
                col('user_settings_email').like('%@ya.%')
            email = F.coalesce(col('user_settings_email'), lit(''))
            user_part = F.split(email, '@')[0]
            norm_user_part = F.regexp_replace(user_part, r'\.', '-')
            norm_mail = F.when(is_yandex_mail, F.concat(norm_user_part, lit('@yandex.ru')))\
                .otherwise(col('user_settings_email'))
            return F.lower(norm_mail)

        cols = [
            col('billing_account_id'),
            replace_with_null('phone'),
            _normalize_email().alias('email'),
            replace_with_null('first_name'),
            replace_with_col('last_name',  col('account_name')),
            replace_with_null('account_name').alias('client_name'),
            replace_with_null('timezone'),
            lit(self._lead_source).alias('lead_source'),
            col('description')
        ]

        if "owner" in preds.columns:
            cols.append(col('owner'))

        if "sort_column" in preds.columns:
            cols.append(col('sort_column'))

        prepared_preds = acq\
            .join(cloud_owners_history, on='puid', how='left')\
            .join(preds, on='billing_account_id', how='inner')\
            .select(cols)

        return prepared_preds

    def _filter_predictions(self, prepared_pred: SparkDataFrame, crm_preds: SparkDataFrame) -> SparkDataFrame:
        filter_statuses = ['in process', 'pending',
                           'new',  'converted', 'assigned', 'recycled']
        existing_crm_preds = crm_preds.filter(
            F.lower(col('status')).isin(filter_statuses))
        if self._only_lead_source:
            existing_crm_preds = (
                existing_crm_preds
                .filter(
                    (col('lead_source') == self._lead_source)
                    | (col('lead_source_crm').isin(['trial', 'var']))
                )
            )
        new_preds: SparkDataFrame = (
            prepared_pred
            .join(existing_crm_preds, how='leftanti', on='billing_account_id')
            .drop_duplicates()

        )

        if 'sort_column' in new_preds.columns:
            new_preds = (
                new_preds
                .sort(col("sort_column").desc())
                .limit(self._leads_daily_limit)
                .drop("sort_column")
            )
        else:
            new_preds = new_preds.limit(self._leads_daily_limit)
        return new_preds.cache()

    # @prepare_for_yt
    def save_to_crm(self) -> tp.Tuple[SparkDataFrame, SparkDataFrame]:
        crm_historical_data = self._historical_data_adapter.historical_preds()
        prepared_preds = self._prepared_predictions()
        filtered_preds = self._filter_predictions(
            prepared_preds, crm_historical_data).cache()
        return filtered_preds, crm_historical_data


def upsale_to_update_leads(df: SparkDataFrame, lead_source: str = 'upsell') -> SparkDataFrame:

    def add_brackets(col: tp.Union[SparkColumn, str]) -> SparkColumn:
        return F.concat(F.lit('["'), col, F.lit('"]'))

    cols = [F.unix_timestamp().alias('Timestamp'),
            add_brackets(col('billing_account_id')).alias(
                'Billing_account_id'),
            col('description').alias('Description'),
            F.coalesce(col('phone'), lit('-')).alias('Phone_1'),
            add_brackets(col('email')).alias('Email'),
            lit(lead_source).alias('Lead_Source'),
            col('lead_source').alias('Lead_Source_Description'),
            col('timezone').alias('Timezone'),
            col('first_name').alias('First_name'),
            col('last_name').alias('Last_name'),
            col('client_name').alias('Account_name')]
    if 'owner' in df.columns:
        cols.append(col('owner').alias('Assigned_to'))
    return df.select(cols)
