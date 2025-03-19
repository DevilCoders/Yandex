/*___DOC___
====Rated Tickets pct
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/rated_tickets_pct.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;
/*
IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;
*/
IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time,$round_period_format;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $rated_tickets_pct() AS 
    DEFINE SUBQUERY $tickets_rated_count() AS
        SELECT 
            `date`,
            count(issue_key) as rated_tickets_count
        FROM (/*
            SELECT 
                issue_key,
                $format_date(DateTime::StartOfWeek(DateTime::FromMicroseconds(CAST (feedback.created_at AS Uint64)))) as `date`,
            FROM `//home/cloud_analytics/import/yc-support/static/cloudsupport_feedback` as feedback
            LEFT JOIN `//home/cloud_analytics/import/yc-support/static/cloudsupport_issues` as issues
            ON feedback.issue_key = issues.startrek_key
            WHERE DateTime::ToDays(DateTime::MakeDatetime(DateTime::StartOfWeek(DateTime::FromMicroseconds(CAST (feedback.created_at AS Uint64)))) - DateTime::MakeDatetime(DateTime::StartOfWeek(DateTime::FromMicroseconds(CAST (issues.created_at AS Uint64))))) / 7 < 4
            */
            SELECT
                $round_period_format('week',resolved_at) as `date`,
                issue_key
            FROM
                `//home/cloud-dwh/data/preprod/cdm/support/dm_yc_support_issues`
            WHERE 
                `feedback_general` not Null
                and $round_period_format('week',resolved_at) < $round_current_time('week') 
                and DateTime::ToDays(DateTime::MakeDatetime(DateTime::StartOfWeek(DateTime::FromSeconds(CAST (created_at AS Uint32)))) -  
                DateTime::MakeDatetime(DateTime::StartOfWeek(DateTime::FromSeconds(CAST (created_at AS Uint32))))) / 7 < 4
        )   
        GROUP BY    
            `date`

    END DEFINE;


    DEFINE SUBQUERY $weeks() AS
        SELECT  
            `date`,
            1 as one
        FROM (
            SELECT
                `date`
            FROM (
                SELECT
                    $dates_range('2018-01-01', $format_date(CurrentUtcDatetime())) as `date`
            )
            FLATTEN BY 
                `date`
        )
        WHERE DateTime::GetDayOfWeekName($parse_date(`date`)) = 'Monday'
    END DEFINE;



    DEFINE SUBQUERY $tickets_all() AS
        SELECT
            issue_key as startrek_key,
            --$format_date(DateTime::StartOfWeek(DateTime::FromMicroseconds(CAST (created_at AS Uint64)))) as `date`,
            $format_date(DateTime::StartOfWeek(DateTime::FromSeconds(CAST (created_at AS Uint32)))) as `date`,
            1 as one
        FROM 
            --`//home/cloud_analytics/import/yc-support/static/cloudsupport_issues`
            `//home/cloud-dwh/data/preprod/cdm/support/dm_yc_support_issues`
    END DEFINE;


    DEFINE SUBQUERY $tickets_all_count() as
        SELECT
            `date`,
            count(startrek_key) as tickets_all_count
        FROM (
            SELECT
                weeks.`date` as `date`,
                startrek_key
            FROM 
                $tickets_all() as tickets_all
            LEFT JOIN
                $weeks() as weeks
            ON tickets_all.one = weeks.one
            WHERE 
                DateTime::ToDays(DateTime::MakeDatetime($parse_date(weeks.`date`)) - DateTime::MakeDatetime($parse_date(tickets_all.`date`))) / 7 < 4
                AND tickets_all.`date` <= weeks.`date`
        )
        GROUP BY 
            `date`
            --- / 7 < 4 ????
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
            rated_tickets_count/(tickets_all_count +0.0) * 4 * 100 as metric_value
        FROM 
            $tickets_rated_count() AS tickets_rated_count
        LEFT JOIN 
            $tickets_all_count() as tickets_all_count
        ON tickets_rated_count.`date` = tickets_all_count.`date`
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $params, 'straight', 'standart', '')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
            ['week'],
            $res_with_metric_growth
        );

    PROCESS $s()


END DEFINE;

EXPORT $rated_tickets_pct;