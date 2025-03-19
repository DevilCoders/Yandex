/*___DOC___
====Marketing Leads
Количество лидов CRM с %%lead_source like 'mkt%'%% созданных за неделю.
<{График
{{iframe frameborder="0" width="70%" height="300px" src="https://charts.yandex-team.ru/preview/editor/hv5twavvj2jqb?metric_name=Marketing%20Leads%20Count&6112cac5-7432-4748-a900-99da3f669075=Marketing%20Leads%20Count&metricName=Marketing%20Leads%20Count&_embedded=1"}}
}>
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/marketing_leads/marketing_leads.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, 
$dates_range, $round_period_datetime, $round_period_date,  $round_current_time, $round_period_format, $format_from_second;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $marketing_leads() AS 
    DEFINE SUBQUERY $res($params) AS
        SELECT
            `date`,
            'mrkt_leads_count' as metric_id,
            'Marketing Leads Count' as metric_name,
            'marketing' as metric_group,
            'marketing' as metric_subgroup,
            '' as metric_unit, 
            0 as is_ytd,
            $params as period,
            count(distinct lead_id) as metric_value
        FROM (
            SELECT
                $round_period_format($params, $format_from_second(date_entered)) as `date`,
                lead_source,
                lead_id
            FROM
                `//home/cloud_analytics/import/crm/leads/leads_cube`
        )
        WHERE 
            lead_source like 'mkt%'
            and if($params = 'year', `date` <= $round_current_time($params),
                `date` < $round_current_time($params))
        GROUP BY 
            `date`
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $params, 'straight', 'standart', '')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $marketing_leads;