USE hahn;

PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
PRAGMA yt.QueryCacheTtl = '1h';
PRAGMA yt.Pool = "cloud_analytics_pool";

$source_table = '//home/cloud_analytics/ml/mql_marketing/by_puids_spax';
$result_table = '//home/cloud_analytics/ml/mql_marketing/by_mal_list';
$mal_table = '//home/cloud_analytics/kulaga/acc_sales_ba_cube';

$uni_lower = ($str) -> ( String::AsciiToLower(Cast(Unicode::ToLower(Cast($str AS Utf8)) AS String)) );

$mal_table =
    SELECT
        $uni_lower(`acc_name`) AS `acc_name_t`,
        ListConcat(AGG_LIST(Cast(`ba_id` AS String)), "; ") AS `ba_id`,
        ListConcat(AGG_LIST(Cast(`inn` AS String)), "; ") AS `mal_inn`,
        ListConcat(AGG_LIST(Cast(`kpp` AS String)), "; ") AS `mal_kpp`
    FROM $mal_table
    WHERE `acc_name` IS NOT NULL
        AND `acc_name` != ''
    GROUP BY `acc_name`
;

INSERT INTO $result_table WITH TRUNCATE
    SELECT *
    FROM $source_table AS st
    LEFT JOIN $mal_table AS mt ON st.`mal_name` = mt.`acc_name_t`
    WHERE `mal_name` != ''
        AND `mal_name` IS NOT NULL
    ORDER BY `date`, `acc_name_t`, `inn`
;
