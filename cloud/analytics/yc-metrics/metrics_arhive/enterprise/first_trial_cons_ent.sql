/*___DOC___
====First Trial Cons Enterprise
Количество биллинг-аккаунтов, активировавших триал на этой неделе (без фрода). Рассматриваются биллинг-аккаунты с сегментом Enterprise.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/enterprise/first_trial_cons_ent.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $first_trial_cons() AS
    DEFINE SUBQUERY $res($param,$period) AS 
        SELECT 
            `date`,
            'first_trial_cons_ent' as metric_id,
            'First Trial Cons Ent' as metric_name,
            'enterprise' as metric_group,
            'enterprise' as metric_subgroup,
            'BAs' as metric_unit, 
            0 as is_ytd,
            $period as period,
            count(distinct(billing_account_id)) as metric_value
        FROM (
            SELECT 
                $round_period_datetime($period,first_first_trial_consumption_datetime) as `date`,
                billing_account_id
            FROM 
                `//home/cloud_analytics/cubes/acquisition_cube/cube`
            WHERE 
                crm_account_segment in ('Enterprise')
                and is_fraud=0
                and if($period = 'year', $round_period_datetime($period,first_first_trial_consumption_datetime) = $round_current_time($period),
                $round_period_datetime($period,first_first_trial_consumption_datetime) < $round_current_time($period))
        )
        GROUP BY `date`
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, '', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth
        );

    PROCESS $s()


END DEFINE;

EXPORT $first_trial_cons;