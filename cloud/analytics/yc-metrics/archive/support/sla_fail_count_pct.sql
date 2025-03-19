/*___DOC___
==== SLA Fail Count pct
Доля комментариев, созданных за неделю и по которым нарушен SLA
Метрика считается отдельно для каждого из 4 видов поддержки: %%premium%%, %%business%%, %%standard%% и %%free%%
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/sla_fail_count_pct.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $sla_fail_count_pct() AS
    DEFINE SUBQUERY $res($support_type) AS
        SELECT 
            `date`,
            'support_sla_fails_count_'|| $support_type as metric_id,
            'Support - SLA Fails Count (' || $support_type || ')' as metric_name,
            'support' as metric_group,
            'support' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            $period as period,
            sum(sla_failed+0.0) / count(startrek_id) * 100 as metric_value
        FROM(
        SELECT
            comments.startrek_id as startrek_id,
            $format_date(DateTime::StartOfWeek(comments.created_at)) as `date`,
            sla_failed,
            billing_id,
            pay
        FROM (SELECT * FROM `//home/cloud_analytics/import/yc-support/static/cloudsupport_comments`) as comments
        LEFT JOIN (SELECT * FROM `//home/cloud_analytics/import/yc-support/static/cloudsupport_issues`) as issues
        ON comments.issue_key = issues.startrek_key
        WHERE 
        --     sla_failed = 1 
        --     -- AND issue_key = 'CLOUDSUPPORT-70756'
            pay = $support_type
        )
        WHERE 1=1
            --`date` < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
        GROUP BY `date`
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

EXPORT $sla_fail_count_pct;