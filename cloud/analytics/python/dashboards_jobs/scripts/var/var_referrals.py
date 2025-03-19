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
    result_table_path = '//home/cloud_analytics/dashboards/CLOUDANA-1117-var-partners-managers/referral_code_usage'
    query = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool='cloud_analytics_pool';

    $date = ($str) -> {{RETURN CAST(SUBSTRING($str, 0, 10) AS DATE)}};
    $result_table = '{result_table_path}';


    $referrers_referrals = (
        --sql
        SELECT
            DISTINCT
            `referral_id`,
            `referrer_id`,
            `created_at`,
            1 AS referral

        FROM `//home/cloud/billing/exported-billing-tables/referral_code_usage_prod` 
        --endsql
    );

    $referrers = (
        --sql
        SELECT
            `referrer_id` AS referral_id,
            `referrer_id`,
            0 AS referral,
            NULL AS `created_at`

        FROM `//home/cloud/billing/exported-billing-tables/referral_code_usage_prod` 
        --endsql
    );

    $referrers_referrals_all = (
        --sql
        SELECT * 
        FROM $referrers_referrals
        UNION ALL
        SELECT * 
        FROM $referrers
        --endsql
    );

    $accounts_names = (
        --sql
        SELECT
            event_time AS referrer_account_created_time,
            billing_account_id,
            account_name
        FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE event='ba_created' -- AND  ba_usage_status != 'service' 
        --endsql
    );


    $referrals = (
        --sql
        SELECT
            accounts_names.account_name AS referrer_account_name,
            accounts_names.referrer_account_created_time as referrer_account_created_time,
            referrers_referrals.*
        FROM $referrers_referrals_all AS referrers_referrals
        INNER JOIN $accounts_names AS accounts_names
        ON referrers_referrals.referrer_id = accounts_names.billing_account_id
        --endsql
    );

    $consumption = (
        --sql
        SELECT
            $date(event_time) as event_time,
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
        WHERE event='day_use' -- AND  ba_usage_status != 'service' 
        --endsql        
    );


    $referral_consumption = (
        --sql
        SELECT *
        FROM $referrals AS referrals
        LEFT JOIN $consumption AS consumption
            ON referrals.referral_id = consumption.billing_account_id
        --endsql
    );

    --sql
    INSERT INTO $result_table WITH TRUNCATE 
    SELECT * 
    FROM $referral_consumption
    --endsql
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