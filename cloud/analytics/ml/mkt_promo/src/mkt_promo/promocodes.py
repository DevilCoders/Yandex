import pyspark.sql.types as T
import pyspark.sql.functions as F
from pyspark.sql.functions import col
from pyspark.sql.session import SparkSession
from pyspark.sql.column import Column as SparkColumn
from pyspark.sql.dataframe import DataFrame as SparkDataFrame


class GetPromocodes:
    """
    A class used to generate leads for `Leads from Vimpelcom_mass` (CLOUDANA-1870)
    ...
    Attributes
    ----------
    offers_path : str
        YT: path to info about promocode offers (exported-billing-old)
    grants_path : str
        YT: path to info about promocode activations (exported-billing-old)
    ba_pii_path : str
        YT: path to table with billing-account contact information (cloud-dwh)
    Methods
    -------
    generate_upsell_and_contact_leads(leads_max_num=40)
        Generates leads for 'upsell' and 'contact more then 70 days'
    """
    # directories on YT
    offers_path = '//home/cloud/billing/exported-billing-tables/monetary_grant_offers_prod'
    grants_path = '//home/cloud/billing/exported-billing-tables/monetary_grants_prod'
    ba_pii_path = '//home/cloud_analytics/import/crm/leads/contact_info'

    def __init__(self, spark: SparkSession) -> None:
        self.spark = spark

    @staticmethod
    def get_uint64_date(colname: str) -> SparkColumn:
        return F.to_date(F.to_timestamp(col(colname).astype(T.LongType())))

    def get_created_promocodes(self) -> SparkDataFrame:
        """Builds SparkDataFrame with main info about created promocodes
        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataFrame with columns:
            * promocode_id
            * duration_days
            * date_created
            * date_expired
            * date_exported
            * amount
            * currency
            * ticket
            * proposed_to
        """

        def get_ticket(colname: str) -> SparkColumn:
            """Parses Yson-column and gets ticket according to weak rules of data structure
            """
            map_schema = "MAP<STRING, STRING>"
            url_regexp = r'http[s]?://[\.\w\-\/]+/'

            col_norm = F.regexp_replace(colname, r'^(\\")|(\\")$|(\\r)|(\\\\)', '')
            col_map = F.from_json(col_norm, map_schema)
            col_reason = F.regexp_replace(col_map['reason'], url_regexp, '')
            col_ticket = col_map['ticket']
            return F.coalesce(col_reason, col_ticket)

        spdf_promocode_created = (
            self.spark
            .read.option('arrow_enabled', 'false').option("parsing_type_v3", "true")
            .schema_hint({'proposed_meta': T.StringType()})
            .yt(self.offers_path)
            .select(
                col('id').alias('promocode_id'),
                (col('duration').astype(T.LongType())/3600/24).alias('duration_days'),
                self.get_uint64_date('created_at').alias('date_created'),
                self.get_uint64_date('expiration_time').alias('date_expired'),
                self.get_uint64_date('export_ts').alias('date_exported'),
                col('initial_amount').alias('amount'),
                col('currency').alias('currency'),
                get_ticket('proposed_meta').alias('ticket'),
                col('proposed_to').alias('proposed_to')
            )
        )
        return spdf_promocode_created

    def get_activated_promocodes(self) -> SparkDataFrame:
        """Builds SparkDataFrame with main info about activated promocodes
        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataFrame with columns:
            * billing_account_id
            * date_start_activation
            * date_end_activation
            * activation_id
            * promocode_id
        """
        spdf_promocode_activated = (
            self.spark.read.yt(self.grants_path)
            .select(
                'billing_account_id',
                self.get_uint64_date('start_time').alias('date_start_activation'),
                self.get_uint64_date('end_time').alias('date_end_activation'),
                col('id').alias('activation_id'),
                col('source_id').alias('promocode_id')
            )
        )
        return spdf_promocode_activated

    def get_personal_data(self) -> SparkDataFrame:
        """Builds SparkDataFrame with contact information about billing-accounts
        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * billing_account_id
            * first_name
            * last_name
            * phone
            * email
            * display_name
        """
        spdf_personal_data = (
            self.spark.read.yt(self.ba_pii_path)
            .select(
                'billing_account_id',
                'first_name',
                'last_name',
                'phone',
                'email',
                'display_name'
            )
        )
        return spdf_personal_data

    def get_promocodes_averall(self) -> SparkDataFrame:
        """Builds SparkDataFrame with averall information about promocodes
        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * promocode_id
            * duration_days
            * date_created
            * date_expired
            * date_exported
            * amount
            * currency
            * ticket
            * proposed_to
            * activation_id
            * date_start_activation
            * date_end_activation
            * billing_account_id
            * first_name
            * last_name
            * phone
            * email
            * display_name
        """
        spdf_promocodes = (
            self.get_created_promocodes()
            .join(self.get_activated_promocodes(), on='promocode_id', how='left')
            .join(self.get_personal_data(), on='billing_account_id', how='left')
            .select(
                'promocode_id',
                'duration_days',
                'date_created',
                'date_expired',
                'date_exported',
                'amount',
                'currency',
                'ticket',
                'proposed_to',
                'activation_id',
                'date_start_activation',
                'date_end_activation',
                'billing_account_id',
                'first_name',
                'last_name',
                'phone',
                'email',
                'display_name'
            )
        )
        return spdf_promocodes
