/*___DOC___
====Conversion Trial -> Paid
Средняя конверсия Trial->Paid за последние 10 недель. Выбирается кол-во конверсий Trial -> Paid за неделю среди биллинг-аккаунтов начавших триал не более чем за 10 недель до недели расчета метрики(биллинг-аккаунты, начавшие платное потребление без триала не учитываются). Это кол-во делится на общее кол-во биллинг-аккаунтов начавших триал не болле чем за 10 недель до недели расчета метрики и умножается на 10.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/SMB/conversion_trial_paid.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $conversion_trial_paid() AS
    DEFINE SUBQUERY $first_paid_cons($params) AS
        SELECT
            billing_account_id,
            min($format_date(DateTime::StartOfWeek($parse_datetime(event_time)))) as start_of_paid_week
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 
            real_consumption_vat > 0 
            and sku_lazy = 0
            and crm_account_segment in ('Mass', 'Medium')
            and case 
                when $params = 'company' then ba_person_type = 'company'
                when $params = 'all' then True
                else True
            end
        GROUP BY 
            billing_account_id
    END DEFINE;

    DEFINE SUBQUERY $first_trial_cons($params) AS
        SELECT
            billing_account_id,
            min($format_date(DateTime::StartOfWeek($parse_datetime(event_time)))) as start_of_trial_week
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 
            trial_consumption_vat > 0 
            and crm_account_segment in ('Mass', 'Medium')
            and case 
                when $params = 'company' then ba_person_type = 'company'
                when $params = 'all' then True
                else True
            end
        GROUP BY 
            billing_account_id
    END DEFINE;

    DEFINE SUBQUERY $ba_created($params) AS
        SELECT 
            billing_account_id,
            $format_date(DateTime::StartOfWeek($parse_datetime(event_time))) as ba_created_week
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 
            event = 'ba_created'
            and crm_account_segment in ('Mass', 'Medium')
            and case 
                when $params = 'company' then ba_person_type = 'company'
                when $params = 'all' then True
                else True
            end
    END DEFINE;

    DEFINE SUBQUERY $ba_stats($params) AS
    SELECT 
        ba_created.billing_account_id as billing_account_id,
        ba_created_week,
        start_of_paid_week,
        start_of_trial_week 
    FROM $ba_created($params) as ba_created
    LEFT JOIN $first_trial_cons($params) as first_trial_cons 
    ON ba_created.billing_account_id = first_trial_cons.billing_account_id
    LEFT JOIN $first_paid_cons($params) as first_paid_cons
    ON ba_created.billing_account_id = first_paid_cons.billing_account_id
    END DEFINE;

    DEFINE SUBQUERY $conversions_by_trial_cohorts($params) AS
    SELECT 
        start_of_paid_week,
        paid.start_of_trial_week as start_of_trial_week,
        paid,
        trial 
    FROM (
        SELECT 
            start_of_paid_week,
            start_of_trial_week,
            count(billing_account_id) as paid
        FROM $ba_stats($params)
        WHERE DateTime::MakeTimestamp($parse_date(start_of_paid_week)) < DateTime::MakeTimestamp($parse_date(start_of_trial_week)) + DateTime::IntervalFromDays(70)
        GROUP BY start_of_paid_week, start_of_trial_week
    ) as paid 
    LEFT JOIN (
        SELECT 
            start_of_trial_week,
            count(billing_account_id) as trial
        FROM 
            $ba_stats($params)
        GROUP BY start_of_trial_week
    ) as trial 
    ON paid.start_of_trial_week = trial.start_of_trial_week
    END DEFINE;

    DEFINE SUBQUERY $res($params) AS
        SELECT 
            start_of_paid_week as `date`,
            'conv_trial_to_paid_'||$params as metric_id,
            'Conversion Trial -> Paid' as metric_name,
            'SMB' as metric_group,
            'SMB' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            'week' as period,
            sum(paid+0.0) / sum(trial+0.0) * 100.0 *10.0 as metric_value
        FROM 
            $conversions_by_trial_cohorts($params)

        GROUP BY start_of_paid_week
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $params, 'straight', 'standart', '')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
            ['all','company'],
            $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $conversion_trial_paid;