/*___DOC___
==== Conversion Trial -> Paid (10 Weeks)
Конверсия из триала в платное потребление для биллинг-аккаунтов начавших свое потребление за 10 недели от даты, на которую расчитывается метрика
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/SMB/conversion_trial_paid_10w.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $conversion_trial_paid_10w() AS
    $first_paid_cons = (
        SELECT
            billing_account_id,
            min($format_date(DateTime::StartOfWeek($parse_datetime(event_time)))) as start_of_paid_week
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 
            real_consumption_vat > 0 
            and sku_lazy = 0
            and crm_account_segment in ('Mass', 'Medium')
        GROUP BY 
            billing_account_id
    );

    $first_trial_cons = (
        SELECT
            billing_account_id,
            min($format_date(DateTime::StartOfWeek($parse_datetime(event_time)))) as start_of_trial_week
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 
            trial_consumption_vat > 0 
            and crm_account_segment in ('Mass', 'Medium')
        GROUP BY 
            billing_account_id
    );

    $ba_created = (
        SELECT 
            billing_account_id,
            $format_date(DateTime::StartOfWeek($parse_datetime(event_time))) as ba_created_week
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 
            event = 'ba_created'
            and crm_account_segment in ('Mass', 'Medium')
    );
    $ba_stats = (
    SELECT 
        ba_created.billing_account_id as billing_account_id,
        ba_created_week,
        start_of_paid_week,
        start_of_trial_week 
    FROM $ba_created as ba_created
    LEFT JOIN $first_trial_cons as first_trial_cons 
    ON ba_created.billing_account_id = first_trial_cons.billing_account_id
    LEFT JOIN $first_paid_cons as first_paid_cons
    ON ba_created.billing_account_id = first_paid_cons.billing_account_id
    );

    DEFINE SUBQUERY $res($param,$period) AS 
        SELECT 
            $format_date(DateTime::MakeDatetime($parse_date(ba_created_week)) + INTERVAL('P70D')) as `date`,
            'conv_trial_to_paid_10w' as metric_id,
            'Conversion Trial -> Paid (10 Weeks)' as metric_name,
            'SMB' as metric_group,
            'SMB' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            $period as period,
            sum(IF(start_of_paid_week IS NULL, 0,1.0))/sum(IF(start_of_trial_week IS NULL, 0,1.0)) * 100.0 as metric_value
        FROM $ba_stats
        GROUP BY ba_created_week
    END DEFINE;
    

    SELECT * FROM 
    $metric_growth($res, '','week', 'straight', 'standart', '')

END DEFINE;

EXPORT $conversion_trial_paid_10w;