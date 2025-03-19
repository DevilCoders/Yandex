USE hahn;

PRAGMA AnsiInForEmptyOrNullableItemsCollections;
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
PRAGMA yt.QueryCacheTtl = '1h';
PRAGMA yt.Pool = "cloud_analytics_pool";

$table_puids = '//home/cloud_analytics/ml/mql_marketing/by_puids_spax';
$result_table_company = '//home/cloud_analytics/ml/mql_marketing/by_companies';
$result_table_puid_contacts = '//home/cloud_analytics/ml/mql_marketing/puid_contacts';


$norm_phone = ($phone) -> {
    $phone_number = Re2::Replace("\\D+")($phone, '');
    $phone_replace_code = If((Length($phone_number) == 11) AND Substring($phone_number, 0, 1) == '8',
                             '7' || Substring($phone_number, 1),
                             $phone_number);
    Return $phone_replace_code
};
$list_phones = ($field) -> ( ListMap(String::SplitToList($field, ";"), $norm_phone) );

$data_by_companies =
    SELECT
        `date`,
        Coalesce(String::SplitToList(`spark_name`, ' || ')[0],`company_name`) AS `company_name`,
        Coalesce(`inn`, '') AS `inn`,
        `company_from_sprav`,
        `company_from_events`,
        `iam_email` AS `puid_email`,
        $norm_phone(Coalesce(`iam_phone`, `sprav_phone`)) AS `puid_phone`,
        ListFilter(ListUniq(ListExtend(
            $list_phones(`iam_phone`),
            $list_phones(`sprav_phone`),
            $list_phones(`company_phones_from_sprav`)
        )), ($x) -> ( Length($x) >= 8 )) AS `all_found_phones`,
        `puid`,
        `visit_count`,
        `visit_count_more2hits`,
        `visit_hits_avg`,
        `visit_hits_avg_more2hits`,
        `events_reg_count`,
        `forms_submitted`,
        `docs_loaded`,
        `visited_from_letter`,
        `mail_promo`,
        `iam_num_created_clouds`
    FROM $table_puids
;

INSERT INTO $result_table_puid_contacts WITH TRUNCATE
    SELECT DISTINCT `puid`, `company_name`, `puid_email`, `puid_phone`, `all_found_phones`
    FROM $data_by_companies
    ORDER BY `puid`, `company_name`
;

INSERT INTO $result_table_company WITH TRUNCATE
    SELECT
        `date`,
        `company_name`,
        `inn`,
        `company_from_sprav`,
        `company_from_events`,
        ListUniq(ListNotNull(Agg_List(`puid`))) AS `related_puids`,
        Count(DISTINCT `puid`) AS `num_found_related_puids`,
        Sum(`visit_count`) AS `visit_count`,
        Sum(`visit_count_more2hits`) AS `visit_count_more2hits`,
        Avg(`visit_hits_avg`) AS `visit_hits_avg`,
        Avg(`visit_hits_avg_more2hits`) AS `visit_hits_avg_more2hits`,
        Sum(`events_reg_count`) AS `averall_events_reg_count`,
        Sum(`forms_submitted`) AS `averall_forms_submitted`,
        Sum(`docs_loaded`) AS `averall_docs_loaded`,
        Sum(`visited_from_letter`) AS `averall_visited_from_letter`,
        Sum(`mail_promo`) AS `mail_promo`,
        Sum(`iam_num_created_clouds`) AS `averall_created_clouds`
    FROM $data_by_companies
    GROUP BY `date`, `company_name`, `inn`, `company_from_sprav`, `company_from_events`
    ORDER BY `date`, `company_name`, `inn`
;
