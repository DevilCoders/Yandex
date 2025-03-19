USE hahn;

PRAGMA yson.DisableStrict;
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
PRAGMA yt.QueryCacheTtl = '1h';
PRAGMA yt.Pool = "cloud_analytics_pool";
PRAGMA yt.MaxJobCount = '96000';
PRAGMA yt.MapJoinLimit = '1G';


-- PARAMETERS
$DAYS_HISTORY = 60;


-- FUNCTIONS
$date_with_lag = ($lag_num) -> ( DateTime::Format('%Y-%m-%d')(CurrentUtcDate() - DateTime::IntervalFromDays($lag_num)) );
$has_item = ($field_name, $item) -> ( Cast(ListHas(Yson::ConvertToUint64List($field_name), $item) AS UInt8) );
$get_field = ($yson, $field) -> {
    $yson = Yson::Parse($yson);
    RETURN IF(Yson::Contains($yson, $field), Yson::LookupString($yson, $field), NULL);
};


-- YT PATHES
$result_by_puids = '//home/cloud_analytics/ml/mql_marketing/by_puids';
$participants_table = '//home/cloud-dwh/data/prod/ods/backoffice/participants';
$applications_table = '//home/cloud-dwh/data/prod/ods/backoffice/applications';
$pii_applications_table = '//home/cloud-dwh/data/prod/ods/backoffice/PII/applications';
$agreements_table = '//home/cloud-dwh/data/prod/ods/iam/passport_users';
$puid_pii_iam_table = '//home/cloud-dwh/data/prod/ods/iam/PII/passport_users';
$metrika_visits_folder = '//home/cloud-dwh/data/prod/ods/metrika/visit_log/';
$sprav_users_table = '//home/sprav/tycoon/production/snapshot/users';
$sprav_roles_table = '//home/sprav/tycoon/production/snapshot/combined_company_acl';
$sprav_pretty_format_table = '//home/sprav/assay/common/company_pretty_format';


-- QUERIES
$metrika_data =
    SELECT
        `puid`,
        HLL(`visit_id`, 18) AS `visit_count`,
        Coalesce(HLL(If(`hits`>=2, `visit_id`, Null), 18), 0) AS `visit_count_more2hits`,
        Math::Round(Coalesce(Avg(`hits`), 0.0), -2) AS `visit_hits_avg`,
        Math::Round(Coalesce(Avg(If(`hits`>=2, `hits`, Null)), 0.0), -2) AS `visit_hits_avg_more2hits`,
        Sum($has_item(`goals_id`, 174723400u)) AS `forms_submitted`,
        Sum($has_item(`goals_id`, 174723388u)) AS `docs_loaded`,
        Sum(Cast((Coalesce(`trafic_source_id`, -1) = 7) AS UInt8)) AS `visited_from_letter`,
        Sum($has_item(`goals_id`, 104725810u)) AS `subscribed_acc_utm`
    FROM Range($metrika_visits_folder, $date_with_lag($DAYS_HISTORY), $date_with_lag(1))
    WHERE `puid` IS NOT NULL
    GROUP BY `puid`
;

$events_data =
    SELECT
        Cast(prts.`puid` AS UInt64) AS `puid`,
        HLL(`event_id`, 18) AS `events_reg_count`
    FROM $applications_table AS apps
    LEFT JOIN $pii_applications_table AS pii_apps ON apps.id = pii_apps.id
    LEFT JOIN $participants_table AS prts ON apps.`participant_id` = prts.`id`
    WHERE prts.`puid` > 0
        AND DateTime::Format('%Y-%m-%d')(`updated_at_msk`) >= $date_with_lag($DAYS_HISTORY)
        AND DateTime::Format('%Y-%m-%d')(`updated_at_msk`) <= $date_with_lag(1)
    GROUP BY prts.`puid`
;

$events_companies =
    SELECT
        Cast(prts.`puid` AS UInt64) AS `puid`,
        ListSortDesc(AGG_LIST_DISTINCT(`participant_company_name`), ($x) -> (LEN($x)))[0] AS `company_from_events`
    FROM $applications_table AS apps
    LEFT JOIN $pii_applications_table AS pii_apps ON apps.id = pii_apps.id
    LEFT JOIN $participants_table AS prts ON apps.`participant_id` = prts.`id`
    WHERE prts.`puid` > 0
    GROUP BY prts.`puid`
;

$puid_info =
    SELECT
        Cast(at.`passport_uid` AS UInt64) AS `puid`,
        Max_By(Cast(`mail_info` AS UInt8), `modified_at`) AS `mail_info`,
        Max_By(Cast(`mail_promo` AS UInt8), `modified_at`) AS `mail_promo`,
        Max_By(`created_clouds`, `modified_at`) AS `created_clouds`,
        Max_By(`email`, `modified_at`) AS `iam_email`,
        Max_By(`phone`, `modified_at`) AS `iam_phone`
    FROM $agreements_table AS at
    LEFT JOIN $puid_pii_iam_table AS ppit ON at.`iam_uid` = ppit.`iam_uid`
    GROUP BY at.`passport_uid`
;

-----------------------------------------------------------------------------------
$sprav_contacts =
    SELECT
        `id` AS `puid`,
        String::AsciiToLower($get_field(`blackbox`, 'default_email')) AS `user_email`,
        $get_field(`blackbox`, 'fio') AS `fio`,
        $get_field(`blackbox`, 'login') AS `login`,
        $get_field(`blackbox`, 'phone') AS `phone`
    FROM $sprav_users_table
;

$sprav_roles =
    SELECT
        `user_id` AS `puid`,
        `object_role`,
        `company_permalink` AS `permalink`
    FROM $sprav_roles_table
;

$sprav_users_all =
    SELECT
        cont.`puid` AS `puid`,
        `user_email`,
        `fio`,
        `login`,
        `phone`,
        `object_role`,
        `permalink`
    FROM $sprav_contacts AS cont
        INNER JOIN $sprav_roles AS rol USING (puid)
;

$sprav_companies_pretty =
    SELECT
        `permalink`,
        `main_rubric_name_ru` AS `company_bussiness`,
        `name` as `company_name`,
        `phones` as `company_phones`,
        '"' || ListConcat(`social_urls`, '"; "') || '"' as `company_social_media`,
        `address` as `company_address`,
        `main_url` as `company_main_url`
    FROM $sprav_pretty_format_table
    WHERE `publishing_status` == 'publish'
;

$spravochnik_companies =
    SELECT DISTINCT
        Cast(usr.`puid` AS UInt64) AS `puid`,
        ListNotNull(AGG_LIST_DISTINCT(`fio`))[0] AS `sprav_fio`,
        ListNotNull(AGG_LIST_DISTINCT(`phone`))[0] AS `sprav_phone`,
        ListNotNull(AGG_LIST_DISTINCT(`company_name`))[0] AS `company_from_sprav`,
        ListNotNull(AGG_LIST_DISTINCT(`object_role`))[0] AS `company_role_from_sprav`,
        ListNotNull(AGG_LIST_DISTINCT(`company_phones`))[0] AS `company_phones_from_sprav`,
        ListNotNull(AGG_LIST_DISTINCT(`company_bussiness`))[0] AS `company_business_from_sprav`,
        ListNotNull(AGG_LIST_DISTINCT(`company_social_media`))[0] AS `company_soc_media_from_sprav`,
        ListNotNull(AGG_LIST_DISTINCT(`company_address`))[0] AS `company_address_from_sprav`,
        ListNotNull(AGG_LIST_DISTINCT(`company_main_url`))[0] AS `company_url_from_sprav`
    FROM $sprav_users_all AS usr
    LEFT JOIN $sprav_companies_pretty AS cmp ON usr.`permalink` = cmp.`permalink`
    WHERE Cast(usr.`puid` AS UInt64) IS NOT NULL
    GROUP BY usr.`puid`
;

INSERT INTO $result_by_puids WITH TRUNCATE
    SELECT DISTINCT *
    FROM (
        SELECT
            Coalesce(met.`puid`, ev.`puid`, pi.`puid`) AS `puid`,
            $date_with_lag(0) AS `date`,
            Coalesce(met.`visit_count`, 0) AS `visit_count`,
            Coalesce(met.`visit_count_more2hits`, 0) AS `visit_count_more2hits`,
            Coalesce(met.`visit_hits_avg`, 0) AS `visit_hits_avg`,
            Coalesce(met.`visit_hits_avg_more2hits`, 0) AS `visit_hits_avg_more2hits`,
            Coalesce(met.`forms_submitted`, 0) AS `forms_submitted`,
            Coalesce(met.`docs_loaded`, 0) AS `docs_loaded`,
            Coalesce(met.`visited_from_letter`, 0) AS `visited_from_letter`,
            Coalesce(met.`subscribed_acc_utm`, 0) AS `subscribed_acc_utm`,
            Coalesce(pi.`mail_info`, 0) AS `mail_info`,
            Coalesce(pi.`mail_promo`, 0) AS `mail_promo`,
            Coalesce(ev.`events_reg_count`, 0) AS `events_reg_count`,
            Coalesce(pi.`created_clouds`, 0) AS `iam_num_created_clouds`,
            pi.`iam_email` AS `iam_email`,
            pi.`iam_phone` AS `iam_phone`,
            `sprav_fio`,
            `sprav_phone`,
            `company_role_from_sprav`,
            `company_phones_from_sprav`,
            `company_business_from_sprav`,
            `company_soc_media_from_sprav`,
            `company_address_from_sprav`,
            `company_url_from_sprav`,
            `company_from_sprav`,
            `company_from_events`,
            Unicode::ToLower(Cast(coalesce(ec.`company_from_events`, sp.`company_from_sprav`) AS Utf8)) AS `company_name`
        FROM $metrika_data AS met
        FULL JOIN $events_data AS ev ON met.`puid` = ev.`puid`
        FULL JOIN $puid_info AS pi ON met.`puid` = pi.`puid`
        LEFT JOIN $events_companies AS ec ON met.`puid` = ec.`puid`
        LEFT JOIN $spravochnik_companies AS sp ON met.`puid` = sp.`puid`

        UNION ALL

        SELECT
            `puid`,
            `date`,
            `visit_count`,
            `visit_count_more2hits`,
            `visit_hits_avg`,
            `visit_hits_avg_more2hits`,
            `forms_submitted`,
            `docs_loaded`,
            `visited_from_letter`,
            `subscribed_acc_utm`,
            `mail_info`,
            `mail_promo`,
            `events_reg_count`,
            `iam_num_created_clouds`,
            `iam_email`,
            `iam_phone`,
            `sprav_fio`,
            `sprav_phone`,
            `company_role_from_sprav`,
            `company_phones_from_sprav`,
            `company_business_from_sprav`,
            `company_soc_media_from_sprav`,
            `company_address_from_sprav`,
            `company_url_from_sprav`,
            `company_from_sprav`,
            `company_from_events`,
            `company_name`
        FROM $result_by_puids
        WHERE `date` < $date_with_lag(0)
            AND `date` >= $date_with_lag(0)
    ) AS tbl
    WHERE `company_name` IS NOT NULL
