/*___DOC___
====Rated Tickets %

Метрика отражает, долю оцененных тикетов, относительно созданных тикетов.
Учитываются только те тикеты, которые были созданы не позже 4 недель от момента расчета метрики.

<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/rated_tickets_pct.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;
IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time,$round_period_format;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $rated_tickets_pct() AS 
    DEFINE SUBQUERY $tickets_rated_count($params) AS
        SELECT 
            `date`,
            Math::Round((sum(fb)/(count(issue_key)+0.0))*100,-2) as rated_tickets_count
        FROM (
            SELECT
                $round_period_format($params,resolved_at) as `date`,
                issue_key,
                if(feedback_general is not NULL,1,0) as fb
            FROM
                `//home/cloud-dwh/data/prod/cdm/support/dm_yc_support_issues`
            WHERE 
                $round_period_format($params,resolved_at) < $round_current_time($params) 
        )   
        GROUP BY    
            `date`

    END DEFINE;


    DEFINE SUBQUERY $res($params) AS
        SELECT
            tickets_rated_count.`date` as `date`,
            'rated_tickets_pct' as metric_id,
            'Support - Rated Tickets Pct' as metric_name,
            'support' as metric_group,
            'support' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            $params as period,
            rated_tickets_count as metric_value
        FROM 
            $tickets_rated_count($params) AS tickets_rated_count
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $params, 'straight', 'standart', '')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
            ['week','month'],
            $res_with_metric_growth
        );

    PROCESS $s()


END DEFINE;

EXPORT $rated_tickets_pct;
