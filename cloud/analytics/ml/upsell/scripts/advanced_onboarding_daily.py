import json
import logging.config
from textwrap import dedent
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
def main(chunk_size: int = 100 * 2 ** 20):
    yql_adapter = YQLAdapter()
    query = yql_adapter.run_query(dedent('''
    PRAGMA AnsiInForEmptyOrNullableItemsCollections;

    ---------------
    -- Variables --
    ---------------
    $leads_update = '//home/cloud_analytics/export/crm/update_call_center_leads/update_leads';
    $adv_onb_history = '//home/cloud_analytics/ml/upsell/advanced_onboarding/adv_onb_history';
    $person_data = '//home/cloud-dwh/data/prod/ods/billing/person_data/person_data';
    $dm_yc_consumption = '//home/cloud-dwh/data/prod/cdm/dm_yc_consumption';

    ---------------
    -- Functions --
    ---------------
    $days_between = ($strdmax, $strdmin) -> {
        $dmax = Cast($strdmax AS Date);
        $dmin = Cast($strdmin AS Date);
        RETURN DateTime::ToDays($dmax - $dmin)
    };

    $str_today =
        DateTime::Format("%Y-%m-%d")(CurrentTzTimeStamp("Europe/Moscow"))
    ;

    ----------------------
    -- Preparing result --
    ----------------------
    $to_title =
        ($string) -> (Cast(Unicode::ToTitle(Cast(String::AsciiToTitle($string) AS Utf8)) AS String));

    $pdn =
        SELECT
            billing_account_id,
            Coalesce(first_name || ' ' || middle_name || ' ' || last_name, first_name || ' ' || last_name,  longname, name) AS display_name,
            first_name,
            Coalesce(last_name, name) AS last_name,
            email,
            phone,
            original_type AS person_type
        FROM (
            SELECT
                billing_account_id,
                original_type,
                person_data,
                Yson::ConvertToString(person_data[original_type]['email']) AS email,
                Yson::ConvertToString(person_data[original_type]['phone']) AS phone,
                $to_title(Yson::ConvertToString(person_data[original_type]['first_name'])) AS first_name,
                $to_title(Yson::ConvertToString(person_data[original_type]['middle_name'])) AS middle_name,
                $to_title(Yson::ConvertToString(person_data[original_type]['last_name'])) AS last_name,
                Yson::ConvertToString(person_data[original_type]['longname']) AS longname,
                Yson::ConvertToString(person_data[original_type]['name']) AS name,
            FROM $person_data
        ) AS t
    ;

    $company_accs_in_paid =
        SELECT
            billing_account_id,
            Min(billing_record_msk_date) AS first_paid_date,
            Max_By(billing_account_person_type_current, billing_record_msk_date) AS person_type,
            Max_By(crm_segment_current, billing_record_msk_date) AS segment
        FROM $dm_yc_consumption
        WHERE billing_account_is_suspended_by_antifraud_current = false
            AND billing_account_is_var_current = false
            AND crm_segment_current = 'Mass'
            AND billing_account_usage_status = 'paid'
            AND billing_account_person_type_current
                IN ('company', 'kazakhstan_company', 'switzerland_nonresident_company')
        GROUP BY billing_account_id
    ;

    $result_leads =
        SELECT
            DateTime::ToSeconds(CurrentTzTimeStamp("Europe/Moscow")) AS `Timestamp`,
            Null AS `CRM_Lead_ID`,
            '["' || cp.billing_account_id || '"]' AS `Billing_account_id`,
            Null AS `Status`,
            'Company type is "' || cp.person_type || '"' AS `Description`,
            'admin' AS `Assigned_to`,
            first_name AS `First_name`,
            last_name AS `Last_name`,
            phone AS `Phone_1`,
            Null AS `Phone_2`,
            '["' || email || '"]' AS `Email`,
            'upsell' AS `Lead_Source`,
            'advanced onboarding' AS `Lead_Source_Description`,
            Null AS `Callback_date`,
            Null AS `Last_communication_date`,
            Null AS `Promocode`,
            Null AS `Promocode_sum`,
            Null AS `Notes`,
            Null AS `Dimensions`,
            Null AS `Tags`,
            Null AS `Timezone`,
            display_name AS `Account_name`
        FROM $company_accs_in_paid AS cp
            LEFT JOIN $pdn AS pd ON cp.`billing_account_id` = pd.`billing_account_id`
        WHERE $days_between($str_today, first_paid_date) = 15
            AND phone IS NOT Null
    ;

    INSERT INTO $leads_update
        SELECT * FROM $result_leads
    ;

    INSERT INTO $adv_onb_history WITH TRUNCATE
        SELECT
            `Timestamp`,
            `CRM_Lead_ID`,
            `Billing_account_id`,
            `Status`,
            `Description`,
            `Assigned_to`,
            `First_name`,
            `Last_name`,
            `Phone_1`,
            `Phone_2`,
            `Email`,
            `Lead_Source`,
            `Lead_Source_Description`,
            `Callback_date`,
            `Last_communication_date`,
            `Promocode`,
            `Promocode_sum`,
            `Notes`,
            `Dimensions`,
            `Tags`,
            `Timezone`,
            `Account_name`
        FROM (
            SELECT * FROM $result_leads
            UNION ALL
            SELECT * FROM $adv_onb_history
        ) AS t
        ORDER BY `Timestamp`
    ;
    '''))

    query.run()
    query.get_results()
    is_success = YQLAdapter.is_success(query)
    if is_success:
        with open('output.json', 'w') as f:
            json.dump({"history_path" : '//home/cloud_analytics/ml/upsell/advanced_onboarding/adv_onb_history'}, f)


if __name__ == "__main__":
    main()
