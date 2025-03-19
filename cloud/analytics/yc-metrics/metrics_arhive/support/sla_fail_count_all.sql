/*___DOC___
==== SLA Fail Count ALL
Количество комментариев, созданных за неделю и по которым нарушен SLA
Метрика считается суммарно для всех видов поддержки
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/sla_fail_count_all.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time,$round_period_format;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $sla_fail_count_all() AS
    DEFINE SUBQUERY $res($support_type,$period) AS
        SELECT 
            `date`,
            'support_sla_fails_count_all' as metric_id,
            'Support - SLA Fails Count ALL' as metric_name,
            'support' as metric_group,
            'support' as metric_subgroup,
            '' as metric_unit, 
            0 as is_ytd,
            $period as period,
            count(startrek_id) as metric_value
        FROM(
        SELECT
            comments.startrek_id as startrek_id,
            --$round_period_datetime($period,comments.created_at) as `date`,
            $round_period_format($period,comments.created_at) as `date`,
            sla_failed,
            billing_id,
            pay
        FROM (SELECT * FROM `//home/cloud_analytics/import/yc-support/static/cloudsupport_comments`) as comments
        LEFT JOIN (SELECT * FROM `//home/cloud_analytics/import/yc-support/static/cloudsupport_issues`) as issues
        ON comments.issue_key = issues.startrek_key
        WHERE 
            sla_failed = 1 
            -- AND issue_key = 'CLOUDSUPPORT-70756'
        )
        WHERE 1=1
            --`date` < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
        GROUP BY `date`
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

EXPORT $sla_fail_count_all;