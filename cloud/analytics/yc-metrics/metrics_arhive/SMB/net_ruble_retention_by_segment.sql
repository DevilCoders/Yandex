/*___DOC___
==== Net Ruble Retention
Динамика роста старых когорт. Насколько выросло платное потребление биллинг-аккаунтов Mass/Medium/Cloud Boost/SMB сегмента, дата регистрации которых раньше даты расчета метрики на 10 недель или больше.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/SMB/net_ruble_retention_by_segment.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $net_ruble_retention_by_segment() AS 
    DEFINE SUBQUERY $cons_raw ($segment) AS
        SELECT 
            $format_date(DateTime::StartOfWeek($parse_datetime(event_time))) as week,
            $format_date(DateTime::StartOfWeek($parse_datetime(first_ba_created_datetime))) as cohort_w,
            real_consumption_vat
        FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 
            if($segment = 'cloud_boost', is_isv = 1, if(
                $segment = 'SMB', crm_account_segment in ('Mass','Medium') ,crm_account_segment = $segment
                )
            )
            AND is_sustainable = 1 
    END DEFINE;

    DEFINE SUBQUERY $revenue_by_cohorts ($segment) AS 
        SELECT 
            week,
            cohort_w,
            sum(real_consumption_vat) as paid
        FROM
            $cons_raw($segment)
        WHERE 
            DateTime::ToDays(DateTime::MakeDatetime($parse_date(week)) - DateTime::MakeDatetime($parse_date(cohort_w))) >= 70
            -- dateDiff('week',toMonday(toDate(first_ba_created_datetime)), toMonday(toDate(event_time))) >=10
        GROUP BY week, cohort_w
    END DEFINE;

    DEFINE SUBQUERY $revenue_by_cohorts_with_prev ($segment) AS 
        SELECT 
            curr.week as week,
            curr.cohort_w as cohort_w,
            curr.paid as paid,
            prev.paid as paid_prev
        FROM 
            $revenue_by_cohorts($segment) as curr 
        LEFT JOIN 
            $revenue_by_cohorts($segment) as prev 
        ON 
            DateTime::MakeDatetime($parse_date(curr.week)) = DateTime::MakeDatetime($parse_date(prev.week)) + INTERVAL('P7D')
            AND curr.cohort_w = prev.cohort_w
        WHERE prev.paid is not null
    END DEFINE;
    
    DEFINE SUBQUERY $res($segment,$period) AS
        SELECT 
            week as `date`,
            'net_ruble_retention_'||$segment as metric_id,
            'Net Ruble Retention_'||$segment  as metric_name,
            'SMB' as metric_group,
            'SMB' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            'week' as period,
            (sum(paid) / sum(paid_prev+0.0)) * 100 as metric_value
        FROM $revenue_by_cohorts_with_prev($segment)
        GROUP BY week
    END DEFINE;

    SELECT * FROM 
        $metric_growth($res, 'Mass','', 'straight', 'goal', '100')
    UNION ALL
    SELECT * FROM 
        $metric_growth($res, 'Medium','', 'straight', 'goal', '100')
    UNION ALL
    SELECT * FROM 
        $metric_growth($res, 'cloud_boost','', 'straight', 'goal', '100')
        UNION ALL
    SELECT * FROM 
        $metric_growth($res, 'SMB','', 'straight', 'goal', '100');
END DEFINE;

EXPORT $net_ruble_retention_by_segment;