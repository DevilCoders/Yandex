/*___DOC___
====Number of tickets rated one all
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/number_of_tickets_rated_one_all.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time,$round_period_format;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $number_of_tickets_rated_one_all() AS 
    DEFINE SUBQUERY $res($param,$period) AS
        SELECT
            -- "issue_key",
            -- "completely",
            feedback.`date` as `date`, 
            'number_of_tickets_rated_one_all'  as metric_id,
            'Number Of Tickets Rated One (All)' as metric_name,
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
                --$format_date(DateTime::StartOfWeek(created_at)) as `date`,
                --$round_period_datetime($period,created_at) as `date`,
                $round_period_format($period,created_at) as `date`,
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
            -- pay = $support_type
            `date` < $round_current_time($period)  --$format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
        GROUP BY feedback.`date`
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

EXPORT $number_of_tickets_rated_one_all;