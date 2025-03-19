/*___DOC___
====Number of tickets rated one
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/number_of_tickets_rated_one.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $number_of_tickets_rated_one() AS 
    DEFINE SUBQUERY $res($support_type) AS
        SELECT
            -- "issue_key",
            -- "completely",
            feedback.`date` as `date`, 
            'number_of_tickets_rated_one_all_'||$support_type as metric_id,
            'Number Of Tickets Rated One (' || $support_type || ')' as metric_name,
            'support' as metric_group,
            'support' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            $period as period,
            sum(if(min_of(rapid,general,completely)=1,1.0,0.0))/count(issue_key) * 100.0 AS metric_value
            -- "description",
            -- "general",
            -- "rapid"
        FROM (
            SELECT
                $format_date(DateTime::StartOfWeek(created_at)) as `date`,
                issue_key,
                rapid,
                general,
                completely
            FROM `//home/cloud_analytics/import/yc-support/static/cloudsupport_feedback`
        ) as feedback
        LEFT JOIN (
            SELECT 
                startrek_key,
                pay 
            FROM `//home/cloud_analytics/import/yc-support/static/cloudsupport_issues`
        ) as issues
        ON feedback.issue_key = issues.startrek_key
        WHERE 
            pay = $support_type
            and `date` < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
        GROUP BY feedback.`date`
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

EXPORT $number_of_tickets_rated_one;