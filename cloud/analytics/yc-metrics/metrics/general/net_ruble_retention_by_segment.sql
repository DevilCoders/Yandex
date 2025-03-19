/*___DOC___
==== Net Ruble Retention
Динамика роста старых когорт. Насколько выросла устойчивая выручка биллинг-аккаунтов Mass/Medium/Cloud Boost/SMB сегмента, дата регистрации которых раньше даты расчета метрики на 10 недель или больше.
Устойчивая выручка - выручка за неделю без ряда sku, потребление по которым начисляется в конце месяца
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/net_ruble_retention_by_segment.sql" formatter="yql"}}
}>
___DOC___*/

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
            $round_period_date($params['period'], billing_record_msk_date) as week,
            $round_period_date($params['period'], msk_event_dt) as cohort_w,
            billing_record_real_consumption_rub_vat
        FROM `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption` as cons
        LEFT JOIN
            `//home/cloud_analytics/export/billing/sku_tags/sku_tags` as sku
            ON cons.sku_id = sku.sku_id
        JOIN    
            (
            SELECT 
                billing_account_id,
                msk_event_dt
            FROM 
                `//home/cloud-dwh/data/prod/cdm/dm_events`
            WHERE 
                event_type = 'billing_account_first_trial_consumption'
            ) as events
        ON 
            cons.billing_account_id = events.billing_account_id
        WHERE 
            if($params['mode'] = 'cloud_boost', billing_account_is_isv = True, if(
                $params['mode'] = 'SMB', crm_segment in ('Mass','Medium') ,crm_segment = $params['mode']
                )
            )
            AND is_sustainable = 1  --собирается из sku_tags
    END DEFINE;

    DEFINE SUBQUERY $revenue_by_cohorts ($params) AS 
        SELECT 
            week,
            cohort_w,
            sum(billing_record_real_consumption_rub_vat) as paid
        FROM
            $cons_raw($params)
        WHERE 
            DateTime::ToDays(DateTime::MakeDatetime($parse_date(week)) - DateTime::MakeDatetime($parse_date(cohort_w))) >= 70
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