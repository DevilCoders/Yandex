import pandas as pd
import logging.config
from typing import List, Tuple
from datetime import datetime, timedelta
import pyspark
import pyspark.sql.types as T
import pyspark.sql.functions as F
from pyspark.sql.window import Window
from pyspark.sql.functions import col, lit
from clan_tools.logging.logger import default_log_config

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def max_by(colname_max, colname_by):
    return F.expr(f"Max_by(`{colname_max}`, `{colname_by}`)")


def nvl(colname, value_if_null):
    return F.when(col(colname).isNull(), value_if_null).otherwise(col(colname)).alias(colname)


def str_today():
    return datetime.now().strftime('%Y-%m-%d')


def add_days(n):
    return (datetime.now()+timedelta(days=n)).strftime('%Y-%m-%d')


class GenerateUpsellLeads:
    """
    A class used to generate leads for 'upsell' and 'contact more then 70 days' (CLOUDANA-1816)

    ...

    Attributes
    ----------
    crm_company_accs_path : str
        YT: path to company accounts from CRM (cloud-analytics)
    dwh_bas_prod_path : str
        YT: path to info about prod billing-accounts (cloud-dwh)
    yc_cons_prod_path : str
        YT: path to table with consumption (cloud-dwh)
    crm_tag_prod_path : str
        YT: path to crm-tags info about prod billing-accounts (cloud-dwh)
    crm_calls_path : str
        YT: path to table with calls (ods-layer cloud-dwh)
    leads_cube_path : str
        YT: path to cdm with CRM leads (operations)
    oppty_cube_path : str
        YT: path to cdm with CRM opportunities (operations)
    sales_cube_path : str
        YT: path to cdm with acc sales (operations)
    staff_info_path : str
        YT: path to table with staff (cloud-dwh)
    staff_pii_info_path : str
        YT: path to table with sensitive info about staff (cloud-dwh)
    person_data_path : str
        YT: path to table with sensitive info about billing-accounts (cloud-analytics)
    db_on_vm_data_path : str
        YT: path to table with info about databases worked on virtual machines (cloud-analytics)

    Methods
    -------
    generate_upsell_and_contact_leads(leads_max_num=40)
        Generates leads for 'upsell' and 'contact more then 70 days'
    """

    crm_company_accs_path = '//home/cloud_analytics/import/crm/business_accounts/data'
    dwh_bas_prod_path = '//home/cloud-dwh/data/prod/ods/billing/billing_accounts'
    yc_cons_prod_path = '//home/cloud-dwh/data/prod/cdm/dm_yc_consumption'
    crm_tag_prod_path = '//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags'
    crm_calls_path = '//home/cloud-dwh/data/prod/ods/crm/crm_calls'
    leads_cube_path = '//home/cloud_analytics/kulaga/leads_cube'
    oppty_cube_path = '//home/cloud_analytics/kulaga/oppty_cube'
    sales_cube_path = '//home/cloud_analytics/kulaga/acc_sales_ba_cube'
    staff_info_path = '//home/cloud-dwh/data/prod/ods/staff/persons'
    staff_pii_info_path = '//home/cloud-dwh/data/prod/ods/staff/PII/persons'
    person_data_path = '//home/cloud_analytics/import/crm/leads/contact_info'
    db_on_vm_data_path = '//home/cloud_analytics/import/network-logs/db-on-vm/data'

    def __init__(self, spark: pyspark.sql.session.SparkSession) -> None:
        """
        Parameters
        ----------
        spark : pyspark.sql.session.SparkSession
            SparkSession to use in pipelines
        """
        self.spark = spark

    def _get_dwh_billing_accounts(self) -> pyspark.sql.dataframe.DataFrame:
        """Builds SparkDataframe with main info about billing-accounts

        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * billing_account_id
            * person_type
            * state
            * usage_status
            * is_suspended_by_antifraud
            * block_reason
            * date_created
            * tech_date
            * segment_current
            * account_owner
        """
        accs_info = (
            self.spark.read.yt(self.crm_tag_prod_path)
            .groupby('billing_account_id', 'segment_current')
            .agg(max_by('account_owner', 'date').alias('account_owner'))
            .distinct()
        )
        dwh_accs = (
            self.spark.read.yt(self.dwh_bas_prod_path)
            .select(
                'billing_account_id',
                'person_type',
                'state',
                'usage_status',
                'is_suspended_by_antifraud',
                'block_reason',
                F.to_date(F.to_timestamp('created_at')).alias('date_created')
            )
            .withColumn('tech_date', F.date_add('date_created', 45))
            .join(accs_info, on='billing_account_id', how='left')
            .withColumn('account_owner', F.when(col('account_owner').isin(self._get_current_staff_logins()), col('account_owner')))
            .cache()
        )
        return dwh_accs

    def _get_all_company_billing_accounts(self) -> pyspark.sql.dataframe.DataFrame:
        """Builds SparkDataframe with main info about only company billing-accounts

        Other criterions:
            * person_type relates to company or billing-account is tagged as 'Comapny (Telesales)' in CRM
            * block reason is not one of 'minimg' or 'manual'
            * usage status is 'paid'
            * billing account is not suspended according to antifraud
            * state is not one of 'inactive', 'payment_not_confirmed' or 'deleted'
            * CRM segment is 'Mass'

        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * billing_account_id
            * person_type
            * state
            * usage_status
            * is_suspended_by_antifraud
            * block_reason
            * date_created
            * tech_date - technical date for counting days from last lead or call (when there wasn't lead or call)
            * segment_current - CRM segment
        """
        crm_company_accs = (
            self.spark.read.yt(self.crm_company_accs_path)
            .select('billing_account_id')
            .distinct()
        )
        dwh_company_accs = (
            self._get_dwh_billing_accounts()
            .filter(col('person_type').isin([
                'company',
                'kazakhstan_company',
                'switzerland_nonresident_company'
            ]))
            .select('billing_account_id')
            .distinct()
        )
        all_company_accs = (
            crm_company_accs
            .union(dwh_company_accs)
            .join(self._get_dwh_billing_accounts(), on='billing_account_id', how='inner')
            .distinct()
            .filter(~col('block_reason').isin(['manual', 'mining']))
            .filter(col('usage_status')=='paid')
            .filter(~col('is_suspended_by_antifraud'))
            .filter(~col('state').isin(['inactive', 'payment_not_confirmed', 'deleted']))
            .filter(col('segment_current')=='Mass')
            .cache()
        )
        return all_company_accs

    def _get_plateau_and_noml_cons_info(self) -> pyspark.sql.dataframe.DataFrame:
        """Builds SparkDataframe with billing-accounts and info about consumption

        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * billing_account_id
            * no_ml_cons - has consumption discrepant to AI services in last two weeks
            * is_plateau - has plateau in consumption in last two weeks
        """
        pd_accs = self._get_all_company_billing_accounts().select('billing_account_id').distinct().toPandas()
        pd_accs['key'] = '0'

        pd_dates = pd.Series(pd.date_range(add_days(-14), add_days(-1))).dt.strftime('%Y-%m-%d')
        pd_dates = pd.DataFrame(pd_dates, columns=['billing_record_msk_date'])
        pd_dates['key'] = '0'

        pd_accs_dates = pd_accs.merge(pd_dates, on='key', how='outer')
        pd_accs_dates = pd_accs_dates[['billing_account_id', 'billing_record_msk_date']]
        spdf_accs_dates = self.spark.createDataFrame(pd_accs_dates)

        ba_cons = (
            self.spark.read.yt(self.yc_cons_prod_path)
            .filter(col('billing_record_msk_date')<str_today())
            .filter(col('billing_record_msk_date')>=add_days(-14))
            .groupby('billing_account_id', 'billing_record_msk_date')
            .agg(
                F.sum('billing_record_real_consumption_rub').alias('billing_record_real_consumption_rub'),
                F.sum((col('sku_service_name')!='cloud_ai').astype('decimal')).alias('no_ml_cons'),
            )
            .join(spdf_accs_dates, on=['billing_account_id', 'billing_record_msk_date'], how='right')
            .select(
                'billing_account_id',
                'billing_record_msk_date',
                nvl('billing_record_real_consumption_rub', 0),
                nvl('no_ml_cons', 0)
            )
            .groupby('billing_account_id')
            .agg(
                (F.sum('no_ml_cons')>0).astype('decimal').alias('no_ml_cons'),
                F.greatest(F.mean('billing_record_real_consumption_rub'), lit(0.01)).alias('avg_consumption'),
                F.greatest(F.stddev('billing_record_real_consumption_rub'), lit(0.01)).alias('std_consumption')
            )
            .withColumn('is_plateau', (1.000*col('std_consumption')/col('avg_consumption')<=0.1).astype('decimal'))
            .select('billing_account_id', 'no_ml_cons', 'is_plateau')
            .cache()
        )
        return ba_cons

    def _get_current_staff_logins(self) -> List[str]:
        """Provides list of logins for current upsell team in staff

        Returns
        ------
        list
            list of logins
        """
        current_staff = (
            self.spark.read.yt(self.staff_info_path)
            .join(
                self.spark.read.yt(self.staff_pii_info_path),
                on='staff_user_id', how='left'
            )
            .filter(col('department_id')==10701)
            .filter(~col('official_is_dismissed'))
            .select('staff_user_login')
        )
        staff_list = current_staff.toPandas()['staff_user_login'].tolist()
        return staff_list

    def _get_last_upsell_leads_info(self) -> pyspark.sql.dataframe.DataFrame:
        """Builds SparkDataframe with historical data about upsell leads

        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * billing_account_id
            * last_lead_date - date of last upsell lead
            * last_actual_manager - last manager login fron list of actual logins from upsell team
        """
        date_of_last_lead = (
            self.spark.read.yt(self.leads_cube_path)
            .filter(col('lead_source')=='upsell')
            .filter(col('billing_account_id').isNotNull())
            .filter(
                col('lead_source_description').isin([
                    "Upsell", "upsell", "contact more then 70 days",
                    "Consumed more than 40k over last 30 days",
                ]) |
                (F.substring('lead_source_description', 0, 19)=='Potential candidate')
            )
            .withColumn('user_name', F.when(col('user_name').isin(self._get_current_staff_logins()), col('user_name')))
            .withColumn('user_name', F.last('user_name', ignorenulls=True).over(
                Window
                .partitionBy('billing_account_id')
                .orderBy('date_entered')
                .rowsBetween(Window.unboundedPreceding, Window.currentRow)
            ))
            .groupby('billing_account_id')
            .agg(
                F.max(F.to_date('date_entered')).alias('last_lead_date'),
                max_by('user_name', 'date_entered').alias('last_actual_manager')
            )
        )
        return date_of_last_lead

    def _get_last_held_calls_info(self) -> pyspark.sql.dataframe.DataFrame:
        """Builds SparkDataframe with historical data about held calls

        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * billing_account_id
            * last_held_call_date - date of last held call
        """
        spdf_leads = (
            self.spark.read.yt(self.leads_cube_path)
            .select(col('billing_account_id').alias('leads_ba_id'), col('lead_id').alias('parent_id'))
            .distinct()
        )

        spdf_oppty = (
            self.spark.read.yt(self.oppty_cube_path)
            .select(col('ba_id').alias('oppty_ba_id'), col('opp_id').alias('parent_id'))
            .distinct()
        )

        spdf_sales = (
            self.spark.read.yt(self.sales_cube_path)
            .select(col('ba_id').alias('sales_ba_id'), col('acc_id').alias('parent_id'))
            .distinct()
        )

        date_of_last_held_call = (
            self.spark.read.yt(self.crm_calls_path)
            .filter(col('deleted') == lit(False))
            .filter(col('crm_call_status') == lit('Held'))
            .filter(col('parent_type').isin(['Accounts', 'Leads', 'Opportunities']))
            .join(spdf_leads, on='parent_id', how='left')
            .join(spdf_oppty, on='parent_id', how='left')
            .join(spdf_sales, on='parent_id', how='left')
            .select(
                col('crm_call_id').alias('id'),
                'parent_type',
                F.to_date(F.to_timestamp(col('date_start_ts').astype(T.LongType())/1000000)).alias('call_date'),
                col('crm_call_status').alias('status'),
                'leads_ba_id',
                'oppty_ba_id',
                'sales_ba_id',
                F.coalesce('leads_ba_id', 'oppty_ba_id', 'sales_ba_id').alias('billing_account_id')
            )
            .distinct()
            .filter(col('billing_account_id').isNotNull())
            .groupby('billing_account_id')
            .agg(F.max('call_date').alias('last_held_call_date'))
            .cache()
        )
        return date_of_last_held_call

    def _get_person_data(self) -> pyspark.sql.dataframe.DataFrame:
        """Builds SparkDataframe with contact information about billing accounts

        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * billing_account_id
            * display_name
            * email
            * first_name
            * last_name
            * phone
        """
        return self.spark.read.yt(self.person_data_path)

    def _get_mdb_data(self) -> pyspark.sql.dataframe.DataFrame:
        """Builds SparkDataframe with information about using mdb by billing accounts according to consumption table

        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * billing_account_id
            * use_mdb
        """
        spdf_mdb = (
            self.spark.read.yt(self.yc_cons_prod_path)
            .filter(col('sku_service_name')=='mdb')
            .groupby('billing_account_id')
            .agg((F.sum('billing_record_cost_rub')>0).astype('decimal').alias('use_mdb'))
        )
        return spdf_mdb

    def _get_db_on_vm_data(self) -> pyspark.sql.dataframe.DataFrame:
        """Builds SparkDataframe with information about used databases on virtual machines (according to launched ports)

        Returns
        ------
        pyspark.sql.dataframe.DataFrame
            SparkDataframe with columns:
            * billing_account_id
            * db_on_vm - comma-separated list of databases (string)
        """
        spdf_db_on_vm = (
            self.spark.read.yt(self.db_on_vm_data_path)
            .filter(col('billing_account_id')!='')
            .groupby('billing_account_id')
            .agg(F.array_join(F.collect_set('db'), ', ').alias('db_on_vm'))
        )
        return spdf_db_on_vm

    def generate_upsell_and_contact_leads(self, leads_max_num: int = 25) -> Tuple[pyspark.sql.dataframe.DataFrame]:
        """Builds SparkDataframes with leads for 'upsell' and 'contact more then 70 days'

        Returns two SparkDataframes: first 'upsell' leads, second 'contact more then 70 days' leads

        Returns
        ------
        Tuple[pyspark.sql.dataframe.DataFrame]
            SparkDataframes with columns:
            * Timestamp
            * CRM_Lead_ID
            * Billing_account_id
            * Status
            * Description
            * Assigned_to
            * First_name
            * Last_name
            * Phone_1
            * Phone_2
            * Email
            * Lead_Source
            * Lead_Source_Description
            * Callback_date
            * Last_communication_date
            * Promocode
            * Promocode_sum
            * Notes
            * Dimensions
            * Tags
            * Timezone
            * Account_name
        """
        spdf_main = (
            self._get_all_company_billing_accounts()
            .join(self._get_plateau_and_noml_cons_info(), on='billing_account_id', how='left')
            .join(self._get_last_upsell_leads_info(), on='billing_account_id', how='left')
            .join(self._get_last_held_calls_info(), on='billing_account_id', how='left')
            .join(self._get_person_data(), on='billing_account_id', how='left')
            .join(self._get_mdb_data(), on='billing_account_id', how='left')
            .join(self._get_db_on_vm_data(), on='billing_account_id', how='left')
            .withColumn('last_lead_date', F.coalesce('last_lead_date', 'tech_date'))
            .withColumn('last_held_call_date', F.coalesce('last_held_call_date', 'tech_date'))
            .select(
                '*',
                F.datediff(lit(str_today()), col('last_lead_date')).alias('last_lead_days_ago'),
                F.datediff(lit(str_today()), col('last_held_call_date')).alias('last_call_days_ago'),
            )
            .withColumn('use_mdb', nvl('use_mdb', 0))
            .withColumn(
                'description',
                F.when(
                    (col('use_mdb')==0) & col('db_on_vm').isNotNull(),
                    F.concat(lit('Client Use BD on VM: '), 'db_on_vm')
                ).otherwise(lit(''))
            )
            .cache()
        )
        logger.info(f'Current company accounts number: {spdf_main.count()}')

        upsell_leads_temp = (
            spdf_main
            .filter(col('is_plateau')==1)
            .filter(col('no_ml_cons')==1)
            .filter(col('last_lead_days_ago')>70)
            .filter(col('last_call_days_ago')>30)
            .sort(col('last_lead_days_ago').desc())
            .limit(leads_max_num)
        )

        curr_timestamp = int((datetime.today()+timedelta(hours=3)).timestamp())
        upsell_leads = (
            upsell_leads_temp
            .select(
                lit(curr_timestamp).alias('Timestamp'),
                lit(None).astype(T.StringType()).alias('CRM_Lead_ID'),
                F.concat(lit('["'), 'billing_account_id', lit('"]')).alias('Billing_account_id'),
                lit(None).astype(T.StringType()).alias('Status'),
                col('description').alias('Description'),
                F.coalesce('last_actual_manager', lit('admin')).alias('Assigned_to'),
                col('first_name').alias('First_name'),
                col('last_name').alias('Last_name'),
                col('phone').alias('Phone_1'),
                lit(None).astype(T.StringType()).alias('Phone_2'),
                F.concat(lit('["'), col('email'), lit('"]')).alias('Email'),
                lit('upsell').alias('Lead_Source'),
                lit('upsell').alias('Lead_Source_Description'),
                lit(None).astype(T.StringType()).alias('Callback_date'),
                lit(None).astype(T.StringType()).alias('Last_communication_date'),
                lit(None).astype(T.StringType()).alias('Promocode'),
                lit(None).astype(T.StringType()).alias('Promocode_sum'),
                lit(None).astype(T.StringType()).alias('Notes'),
                lit(None).astype(T.StringType()).alias('Dimensions'),
                lit(None).astype(T.StringType()).alias('Tags'),
                lit('').alias('Timezone'),
                col('display_name').alias('Account_name')
            )
        )
        contact_more_than_70_days = (
            spdf_main
            .join(upsell_leads_temp, on='billing_account_id', how='leftanti')
            .filter(col('last_held_call_date').isNotNull())
            .filter(col('last_lead_days_ago')>70)
            .filter(col('last_call_days_ago')>70)
            .sort(col('last_call_days_ago').desc())
            .limit(leads_max_num)
            .select(
                lit(curr_timestamp).alias('Timestamp'),
                lit(None).astype(T.StringType()).alias('CRM_Lead_ID'),
                F.concat(lit('["'), 'billing_account_id', lit('"]')).alias('Billing_account_id'),
                lit(None).astype(T.StringType()).alias('Status'),
                col('description').alias('Description'),
                F.coalesce('account_owner', lit('admin')).alias('Assigned_to'),
                col('first_name').alias('First_name'),
                col('last_name').alias('Last_name'),
                col('phone').alias('Phone_1'),
                lit(None).astype(T.StringType()).alias('Phone_2'),
                F.concat(lit('["'), col('email'), lit('"]')).alias('Email'),
                lit('upsell').alias('Lead_Source'),
                lit('contact more then 70 days').alias('Lead_Source_Description'),
                lit(None).astype(T.StringType()).alias('Callback_date'),
                lit(None).astype(T.StringType()).alias('Last_communication_date'),
                lit(None).astype(T.StringType()).alias('Promocode'),
                lit(None).astype(T.StringType()).alias('Promocode_sum'),
                lit(None).astype(T.StringType()).alias('Notes'),
                lit(None).astype(T.StringType()).alias('Dimensions'),
                lit(None).astype(T.StringType()).alias('Tags'),
                lit('').alias('Timezone'),
                col('display_name').alias('Account_name')
            )
        )
        logger.info(f'Prepared: {upsell_leads.count()} upsell and {contact_more_than_70_days.count()} contact leads')
        return upsell_leads, contact_more_than_70_days
