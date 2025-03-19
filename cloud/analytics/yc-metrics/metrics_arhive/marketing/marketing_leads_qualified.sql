/*___DOC___
==== Marketing Leads Qualified
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/marketing/marketing_leads_qualified.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time,$round_period_format,$format_from_second;
IMPORT time_series SYMBOLS $metric_growth;
IMPORT tables SYMBOLS $last_non_empty_table;

DEFINE SUBQUERY $marketing_leads_qualified() AS 
    $mkt_leads = (
        SELECT 
            lead_id
        FROM
            `//home/cloud_analytics/import/crm/leads/leads_cube`
        WHERE 
            lead_source like 'mkt%'
    );

    DEFINE SUBQUERY $get_id($period) AS 
        SELECT 
            id,
            --$format_date(DateTime::StartOfWeek(DateTime::FromSeconds(CAST (date_created AS Uint32)))) as `date`,
            $round_period_format($period, $format_from_second(date_created)) as `date`
        FROM
            $last_non_empty_table('//home/cloud_analytics/dwh/raw/crm/leads_audit')
        WHERE 
            parent_id in $mkt_leads
            AND field_name = 'status'
            AND after_value_string = 'Converted'
    END DEFINE;

    DEFINE SUBQUERY $res($param,$period) AS

        SELECT
            `date`,
            'mrkt_leads_count_qualified' as metric_id,
            'Marketing Leads Count (Qualified)' as metric_name,
            'marketing' as metric_group,
            'marketing' as metric_subgroup,
            '' as metric_unit, 
            0 as is_ytd,
            $period as period,
            count(id) as metric_value
        FROM $get_id($period)
        GROUP BY 
            `date`
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

EXPORT $marketing_leads_qualified;