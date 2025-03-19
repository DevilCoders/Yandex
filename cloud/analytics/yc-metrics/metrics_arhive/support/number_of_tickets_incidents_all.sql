/*___DOC___
====Number of Tickets-Incidents ALL
Кол-во тикетов с типом "инцидент", созданнных за неделю.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/number_of_tickets_incidents_all.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time,$round_period_format;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $number_of_tickets_incidents_all() AS
    DEFINE SUBQUERY $incident_issues($period) AS 
        SELECT
            --$format_date(DateTime::StartOfWeek(created_at)) as `date`,
            --$round_period_datetime($period,created_at) as `date`,
            $round_period_format($period,created_at) as `date`,
            startrek_key
        FROM
            `//home/cloud_analytics/import/yc-support/static/cloudsupport_issues`
        WHERE 
            type = 'incident'
    END DEFINE;

    DEFINE SUBQUERY $res($param,$period) AS
        SELECT 
            `date`,
            'support_cnt_incidents_all' as metric_id,
            'Support - Number of Tickets-Incidents All' as metric_name,
            'support' as metric_group,
            'support' as metric_subgroup,
            '' as metric_unit, 
            0 as is_ytd,
            $period as period,
            count(startrek_key) as metric_value
        FROM $incident_issues($period)
        WHERE
            if($period = 'year', `date` <= $round_current_time($period),
                `date` < $round_current_time($period))
        GROUP BY
            `date`
    END DEFINE;

    
    DEFINE SUBQUERY $res_with_metric_growth($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, '', $period, 'inverse', 'standart', '')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $number_of_tickets_incidents_all;