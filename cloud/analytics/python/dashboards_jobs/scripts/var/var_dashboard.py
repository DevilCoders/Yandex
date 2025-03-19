import json
import logging.config
from textwrap import dedent
import click
from clan_tools import utils
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.utils.conf import read_conf
from clan_tools.utils.timing import timing

config = read_conf('config/logger.yml')
logging.config.dictConfig(config)
logger = logging.getLogger(__name__)

@timing
@click.command()
@click.option('--write', is_flag=True)
def main(write):
    yql_adapter = YQLAdapter()
    result_table_path = '//home/cloud_analytics/dashboards/CLOUDANA-1117-var-partners-managers/var_dashboard'
    query = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    
    $date = ($str) -> {{RETURN CAST(SUBSTRING($str, 0, 10) AS DATE)}};
    $dimensions = '//home/cloud_analytics/dwh/cdm/dim_crm_account';

    $result_table_path = '{result_table_path}';


    $master_account_names = (
        --sql
        SELECT  DISTINCT
                billing_account_id as billing_account_id, 
                account_name       as master_account_name
        FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE event='ba_created' AND  ba_usage_status != 'service' AND is_var = 'var' 
             AND ((master_account_id = '') or (master_account_id is null))
             AND (billing_account_id is not null)  
             AND (billing_account_id != '')  
        --endsql
    );


    $consumption_raw = (
                --sql
                SELECT
                    $date(event_time) as event_time,
                    if((master_account_id is null) or (master_account_id=''), 
                        billing_account_id,
                        master_account_id) as master_account_id,
                    nvl(billing_account_id, '')
                        in (select nvl(billing_account_id, '')
                            from $master_account_names) as is_partner,
                    billing_account_id,
                    account_name,
                    service_name,
                    real_consumption                    as paid,
                    br_var_reward_rub                   as var_reward,
                    br_credit_grant_rub                 as credit_grant,
                    br_credit_service_rub               as credit_service,
                    br_credit_cud_rub                   as credit_cud,
                    br_credit_volume_incentive_rub      as credit_volume_incentive,
                    br_credit_disabled_rub              as credit_disabled,
                    br_credit_trial_rub                 as credit_trial,

                    real_consumption_vat                as paid_vat,
                    br_var_reward_rub_vat               as var_reward_vat,
                    br_credit_grant_rub_vat             as credit_grant_vat,
                    br_credit_service_rub_vat           as credit_service_vat,
                    br_credit_cud_rub_vat               as credit_cud_vat,
                    br_credit_volume_incentive_rub_vat  as credit_volume_incentive_vat,
                    br_credit_disabled_rub_vat          as credit_disabled_vat,
                    br_credit_trial_rub_vat             as credit_trial_vat,

                    $date(first_first_paid_consumption_datetime) as first_paid_consumption_datetime
                FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
                WHERE event='day_use' AND  ba_usage_status != 'service' AND is_var = 'var'
                --endsql        
    );

    --sql
    $consumption = (
        SELECT
            event_time,
            master_account_id,
            is_partner,
            billing_account_id,
            account_name,
            service_name,
            sum(paid) as paid,
            sum(var_reward) as var_reward,
            sum(credit_grant) as credit_grant,
            sum(credit_service) as credit_service,
            sum(credit_cud) as credit_cud,
            sum(credit_volume_incentive) as credit_volume_incentive,
            sum(credit_disabled) as credit_disabled,
            sum(credit_trial) as credit_trial,
            sum(paid_vat) as paid_vat,
            sum(var_reward_vat) as var_reward_vat,
            sum(credit_grant_vat) as credit_grant_vat,
            sum(credit_service_vat) as credit_service_vat,
            sum(credit_cud_vat) as credit_cud_vat,
            sum(credit_volume_incentive_vat) as credit_volume_incentive_vat,
            sum(credit_disabled_vat) as credit_disabled_vat,
            sum(credit_trial_vat) as credit_trial_vat,
            min(first_paid_consumption_datetime) as first_paid_consumption_datetime
        FROM $consumption_raw
        GROUP BY event_time,
                 master_account_id,
                 is_partner,
                 billing_account_id,
                 account_name,
                 service_name
    );


    $consumption_master_account_name = (
        --sql
        SELECT  master_account_names.billing_account_id                 as master_account_id,
                master_account_names.master_account_name                as master_account_name,
                if(consumption.is_partner, 1, 0)                        as is_partner,
                nvl(event_time, CurrentUtcDate())                       as event_time,
                nvl(consumption.billing_account_id, 'no_subaccount')    as billing_account_id,
                nvl(account_name, 'no_subaccount')                      as account_name,
                nvl(service_name, 'not_applicable')                     as service_name,

                nvl(paid, 0)                                            as paid,           
                nvl(var_reward, 0)                                      as var_reward,     
                nvl(credit_grant, 0)                                    as credit_grant,       
                nvl(credit_service, 0)                                  as credit_service,         
                nvl(credit_cud, 0)                                      as credit_cud,     
                nvl(credit_volume_incentive, 0)                         as credit_volume_incentive,                  
                nvl(credit_disabled, 0)                                 as credit_disabled,          
                nvl(credit_trial, 0)                                    as credit_trial,  

                nvl(paid_vat, 0)                                        as paid_vat,
                nvl(var_reward_vat, 0)                                  as var_reward_vat,
                nvl(credit_grant_vat, 0)                                as credit_grant_vat,
                nvl(credit_service_vat, 0)                              as credit_service_vat,
                nvl(credit_cud_vat, 0)                                  as credit_cud_vat,
                nvl(credit_volume_incentive_vat, 0)                     as credit_volume_incentive_vat,
                nvl(credit_disabled_vat, 0)                             as credit_disabled_vat,
                nvl(credit_trial_vat, 0)                                as credit_trial_vat,

                first_paid_consumption_datetime
        FROM $master_account_names as master_account_names
        LEFT JOIN $consumption as consumption
            ON consumption.master_account_id = master_account_names.billing_account_id
        --endsql
    );


    $partners_converted_date = (
        --sql
        SELECT billing_account_id         as master_account_id, 
            MIN($date(date_qualified))    as converted_date
        FROM `//home/cloud_analytics/lunin-dv/dashboard_tables/var_info`
        WHERE LENGTH(date_qualified) >= 10 
        GROUP BY billing_account_id
        --endsql
    );



    $consumption_converted = (
        --sql
        SELECT consumption_master_account_name.*, 
               partners_converted_date.converted_date as converted_date
        FROM $consumption_master_account_name as consumption_master_account_name
        LEFT JOIN $partners_converted_date as partners_converted_date
            ON partners_converted_date.master_account_id = consumption_master_account_name.master_account_id
        --endsql
    );



    $partners_managers = (
        --sql
        SELECT partner_manager, billing_account_id as master_account_id
        FROM `//home/cloud_analytics/dashboards/CLOUDANA-1117-var-partners-managers/partners-managers`
        --endsql
    );


    $consumption_with_partner_manager = (
        --sql
        SELECT consumption_converted.*, 
               partners_managers.partner_manager as partner_manager
        FROM  $consumption_converted as consumption_converted
        LEFT JOIN $partners_managers as partners_managers
        ON consumption_converted.master_account_id = partners_managers.master_account_id
        --endsql
    );


    $consumption_with_dimmensions = (
        --sql
        SELECT consumption_with_partner_manager.*, 
               dimensions.dimension_name as partner_dimension
        FROM  $consumption_with_partner_manager as consumption_with_partner_manager
        LEFT JOIN $dimensions as dimensions
        ON consumption_with_partner_manager.master_account_id = dimensions.billing_account_id
        --endsql
    );

    INSERT INTO $result_table_path WITH TRUNCATE
    SELECT *
    FROM $consumption_with_dimmensions
    '''))

    YQLAdapter.attach_files(utils.__file__, 'yql', query)
    query.run()
    query.get_results()
    is_success = YQLAdapter.is_success(query)
    if is_success:
         with open('output.json', 'w') as f:
            json.dump({"table_path" : result_table_path }, f)


  
    


if __name__ == "__main__":
    main()