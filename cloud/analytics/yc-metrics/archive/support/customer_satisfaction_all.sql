/*___DOC___
====СSAT ALLL
Customer Satisfaction. Считается как процент итоговых оценок 4 и 5 от кол-ва всех оценок, поставленных за неделю. Под итоговой оценкой понимается **минимальная** оценка из %%general%%, %%rapid%% и %%completely%%
Метрика считается без разбивки на виды поддержки.

<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/customer_satisfaction_all.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $customer_satisfaction_all() AS 
    DEFINE SUBQUERY $res($support_type) AS
        SELECT
            -- "issue_key",
            -- "completely",
            feedback.`date` as `date`, 
            'support_csat_all' as metric_id,
            'Support - CSAT ALL' as metric_name,
            'support' as metric_group,
            'support' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            $period as period,
            sum(if(min_of(rapid,general,completely)>=4,1.0,0.0))/count(issue_key) * 100.0 AS metric_value
            -- "description",
            -- "general",
            -- "rapid"
        FROM (
            SELECT
                --$format_date(DateTime::StartOfWeek(created_at)) as `date`,
                $round_period_date($period,created_at) as `date`,
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
			`date` < $round_current_time($period) --$format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))            
        GROUP BY feedback.`date`
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

EXPORT $customer_satisfaction_all;