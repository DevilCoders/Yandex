/*___DOC___
==== Net Ruble Retention
Динамика роста старых когорт. Насколько выросло платное потребление биллинг-аккаунтов Mass/Medium/Cloud Boost/SMB сегмента, дата регистрации которых раньше даты расчета метрики на 10 недель или больше.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/SMB/net_ruble_retention_by_segment.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

$format_name = ($name) -> { RETURN Unicode::ToLower(CAST(coalesce($name,'') as Utf8)) };

$script = @@#python
import itertools
def product(x):
    return itertools.product(*x, repeat=1)
@@;

$product = Python3::product(
    Callable<(List<List<String?>>)->List<List<String?>>>,
    $script
);

$make_param_dict = ($x) -> {
    RETURN 
        ListMap(
            $x,
            ($x) -> {
                RETURN ToDict(
                    AsList(
                        AsTuple('period', coalesce($x[0],'')),
                        AsTuple('mode', coalesce($x[1],''))
                    )
                )
            }
        )
};


DEFINE SUBQUERY $net_ruble_retention_by_segment() AS 
    DEFINE SUBQUERY $cons_raw ($params) AS
        SELECT 
            --$format_date(DateTime::StartOfWeek($parse_datetime(event_time))) as week,
            $round_period_datetime($params['period'], event_time) as week,
            $round_period_datetime($params['period'], first_ba_created_datetime) as cohort_w,
            --$format_date(DateTime::StartOfWeek($parse_datetime(first_ba_created_datetime))) as cohort_w,
            real_consumption_vat
        FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 
            if($params['mode'] = 'cloud_boost', is_isv = 1, if(
                $params['mode'] = 'SMB', crm_account_segment in ('Mass','Medium') ,crm_account_segment = $params['mode']
                )
            )
            AND is_sustainable = 1 
    END DEFINE;

    DEFINE SUBQUERY $revenue_by_cohorts ($params) AS 
        SELECT 
            week,
            cohort_w,
            sum(real_consumption_vat) as paid
        FROM
            $cons_raw($params)
        WHERE 
            DateTime::ToDays(DateTime::MakeDatetime($parse_date(week)) - DateTime::MakeDatetime($parse_date(cohort_w))) >= 70
            -- dateDiff('week',toMonday(toDate(first_ba_created_datetime)), toMonday(toDate(event_time))) >=10
        GROUP BY week, cohort_w
    END DEFINE;

    DEFINE SUBQUERY $revenue_by_cohorts_with_prev ($params) AS 
        SELECT 
            curr.week as week,
            curr.cohort_w as cohort_w,
            curr.paid as paid,
            prev.paid as paid_prev,
            'week' as p
        FROM 
            $revenue_by_cohorts($params) as curr 
        LEFT JOIN 
            $revenue_by_cohorts($params) as prev 
        ON 
            DateTime::MakeDatetime($parse_date(curr.week)) = DateTime::MakeDatetime($parse_date(prev.week)) + INTERVAL('P7D')
            AND curr.cohort_w = prev.cohort_w
        WHERE prev.paid is not null
        UNION ALL
        SELECT 
            curr.week as week,
            curr.cohort_w as cohort_w,
            curr.paid as paid,
            prev.paid as paid_prev,
            'month' as p
        FROM 
            $revenue_by_cohorts($params) as curr 
        LEFT JOIN 
            $revenue_by_cohorts($params) as prev 
        ON 
            DateTime::MakeDate($parse_date(curr.week)) = DateTime::MakeDate(DateTime::ShiftMonths($parse_date(prev.week), 1))
            AND curr.cohort_w = prev.cohort_w
        WHERE prev.paid is not null
    END DEFINE;
    
    DEFINE SUBQUERY $res($params) AS
        SELECT 
            week as `date`,
            'net_ruble_retention_'||$format_name($params['mode'])  as metric_id,
            'Net Ruble Retention_'||CAST($params['mode'] as Utf8)  as metric_name,
            'SMB' as metric_group,
            'SMB' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            CAST($params['period'] AS Utf8) as period,
            (sum(paid) / sum(paid_prev+0.0)) * 100 as metric_value
        FROM $revenue_by_cohorts_with_prev($params)
        WHERE p = $params['period']
        GROUP BY week
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $params, 'straight', 'goal', '100')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
        $make_param_dict(
            $product(
                AsList(
                    AsList(
                        'week',
                        'month'
                    ), 
                    AsList(
                        'Mass',
                        'Medium',
                        'cloud_boost',
                        'SMB'
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $net_ruble_retention_by_segment;