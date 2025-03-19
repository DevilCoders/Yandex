/*___DOC___
====Conversion BA -> Trial (Company)
Конверсия в первое триальное потребление для недельной корогты созданных Billing Account типа Company.
Метрика показывает, у скольких Billing Account было триальное потребление в ту же календарную неделю, в которую эти Billing Account были созданы.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/marketing/conversion_ba_trial.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $conversion_ba_trial() AS
    DEFINE SUBQUERY $new_trial($params) AS
        SELECT
            $round_period_datetime($params['period'],event_time) as `date`,
            if($format_date($parse_datetime(event_time)) <= $format_date($parse_datetime(first_first_trial_consumption_datetime))
                AND $format_date($parse_datetime(first_first_trial_consumption_datetime)) < $format_date(DateTime::MakeDatetime(DateTime::StartOfWeek($parse_datetime(event_time))) + INTERVAL('P7D')), 1, 0) as trial,
            billing_account_id
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 
            event = 'ba_created'
            and ba_person_type = 'company'
            and is_fraud = 0
    END DEFINE;

    DEFINE SUBQUERY $res($params) AS
        SELECT 
            `date`,
            'conversion_ba_trial' as metric_id,
            'Conversion BA =>Trial' as metric_name,
            'marketing' as metric_group,
            'marketing' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            cast($params['period'] as Utf8) as period,
            sum(trial)/(count(distinct billing_account_id) + 0.0) * 100 as metric_value
        FROM 
            $new_trial($params)
        GROUP BY 
            `date`
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $params, 'straight', 'standart', '')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
            AsList(ToDict(AsList(AsTuple('period','week')))),
            $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $conversion_ba_trial;