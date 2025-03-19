/*___DOC___
====Number of Tickets-Incidents
Кол-во тикетов с типом "инцидент", созданнных за неделю, в разбивке по типам поддержки.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/number_of_tickets_incidents.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $number_of_tickets_incidents() AS
    DEFINE SUBQUERY $res($support_type) AS
        SELECT 
            `date`,
            'support_cnt_incidents_'|| $support_type as metric_id,
            'Support - Number of Tickets-Incidents (' || $support_type || ')'as metric_name,
            'support' as metric_group,
            'support' as metric_subgroup,
            '' as metric_unit, 
            0 as is_ytd,
            $period as period,
            count(startrek_key) as metric_value
        FROM (
            SELECT
                $format_date(DateTime::StartOfWeek(created_at)) as `date`,
                startrek_key
            FROM
                `//home/cloud_analytics/import/yc-support/static/cloudsupport_issues`
            WHERE 
                type = 'incident'
                AND pay = $support_type
        )
        WHERE
            `date` < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
        GROUP BY
            `date`
    END DEFINE;

    SELECT * FROM
    $metric_growth($res, 'free', 'inverse','standart','')

    UNION ALL

    SELECT * FROM
    $metric_growth($res, 'standard', 'inverse','standart','')

    UNION ALL

    SELECT * FROM
    $metric_growth($res, 'business', 'inverse','standart','')

    UNION ALL
    
    SELECT * FROM
    $metric_growth($res, 'premium', 'inverse','standart','')

END DEFINE;

EXPORT $number_of_tickets_incidents;