/*___DOC___
====Plan-Fact Revenue Enterprise YTD
Отношение фактической выручки к запланированной для сегменета Enterprise с начала года.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/eterprise/plan_fact_revenue_ent.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $plan_fact_revenue_ent() AS
    DEFINE SUBQUERY $cons_fact($period) AS 
        SELECT 
            --$format_date(DateTime::StartOfWeek($parse_datetime(event_time))) as week,
            $round_period_datetime($period,event_time) as per,
            real_consumption_vat
        FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 
            event = 'day_use'
            AND crm_account_segment = 'Enterprise'
            AND if($period = 'year', $round_period_datetime($period,event_time) = $round_current_time($period),
            $round_period_datetime($period,event_time) < $round_current_time($period))
            AND $round_period_datetime('year',event_time) = $round_current_time('year')
            --AND $format_date(DateTime::StartOfWeek($parse_datetime(event_time))) < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
            --AND $format_date(DateTime::StartOfYear($parse_datetime(event_time))) = $format_date(DateTime::StartOfYear(CurrentUtcDatetime()))
    END DEFINE;

    DEFINE SUBQUERY $agg_fact($period) AS
        SELECT 
            per,
            sum(real_consumption_vat) as real_consumption_vat
        FROM 
            $cons_fact($period)
        GROUP BY 
            per
    END DEFINE;

    DEFINE SUBQUERY $cons_plan($period) AS 
        SELECT 
            plan_optim,
            --$format_date(DateTime::StartOfWeek($parse_date(`date`))) as week
            $round_period_date($period,`date`) as per
        FROM 
            `//home/cloud_analytics/marketing/plan_daily_2021`
        WHERE 
            --$format_date(DateTime::StartOfYear($parse_date(`date`))) = $format_date(DateTime::StartOfYear(CurrentUtcDatetime()))
            $round_period_date('year',`date`) = $round_current_time('year')
            AND segment = 'Ent'
    END DEFINE;

    DEFINE SUBQUERY $agg_plan($period) AS
        SELECT 
            sum(plan_optim) as plan_optim,
            per
        FROM    
            $cons_plan($period)
        GROUP BY 
            per
    END DEFINE;

    DEFINE SUBQUERY $plan_fact($period) AS
        SELECT 
            fact.per as per,
            plan_optim,
            real_consumption_vat
        FROM 
            $agg_fact($period) as fact 
        JOIN 
            $agg_plan($period) as plan 
        ON 
            DateTime::MakeDatetime($parse_date(fact.per)) = DateTime::MakeDatetime($parse_date(plan.per))
        ORDER BY 
            per
    END DEFINE;

    DEFINE SUBQUERY $plan_fact_running_total($period) AS
        SELECT
            per,
            SUM(plan_optim) OVER w1 AS plan_optim_rt,
            SUM(real_consumption_vat) OVER w1 AS real_consumption_vat_rt    
        FROM $plan_fact($period)
        WINDOW
            w1 AS (ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
    END DEFINE;

    DEFINE SUBQUERY $res($param,$period) AS
        SELECT 
            per as `date`,
            'plan_fact_revenue_ent' as metric_id,
            'Plan Fact Revenue Running Total Ent' as metric_name,
            'general' as metric_group,
            'general' as metric_subgroup,
            '%' as metric_unit, 
            1 as is_ytd,
            $period as period,
            (real_consumption_vat_rt/plan_optim_rt+0.0) * 100 AS metric_value
        FROM $plan_fact_running_total($period)
    END DEFINE;

    /*SELECT * FROM 
        $metric_growth($res, '', 'straight', 'goal', '100');*/

    DEFINE SUBQUERY $res_with_metric_growth($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, '', $period, 'straight', 'goal', '100')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $plan_fact_revenue_ent;