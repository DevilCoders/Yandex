/*___DOC___
====Plan-Fact Revenue YTD
Отношение фактической выручки к запланированной с начала календарного года.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/plan_fact_revenue.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $plan_fact_revenue_product() AS
    DEFINE SUBQUERY $cons_fact($param,$period) AS 
        SELECT 
            --$format_date(DateTime::StartOfWeek($parse_datetime(event_time))) as week,
            $round_period_datetime($period,event_time) as per,
            real_consumption_vat
        FROM `//home/cloud_analytics/cubes/acquisition_cube/cube` as acube
        LEFT JOIN
            `//home/cloud_analytics/export/billing/sku_tags/sku_tags` as sku
            ON acube.sku_id = sku.sku_id
        WHERE 
            event = 'day_use'
--            AND $format_date(DateTime::StartOfWeek($parse_datetime(event_time))) < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
--            AND $format_date(DateTime::StartOfYear($parse_datetime(event_time))) = $format_date(DateTime::StartOfYear(CurrentUtcDatetime()))
            AND if($period = 'year', $round_period_datetime($period,event_time) = $round_current_time($period),
                $round_period_datetime($period,event_time) < $round_current_time($period))
            AND $round_period_datetime('year',event_time) = $round_current_time('year')
            AND CASE
                    WHEN $param = 'DataPlatform' then sku.service_group = 'Data Storage and Analytics' and sku.service_name in ('datalens','mdb')
                    WHEN $param = 'ML_AI' then sku.service_group = 'ML and AI'
                    WHEN $param = 'Kubernetes' then sku.service_name = 'mk8s' 
                    WHEN $param = 'Serverless' then sku.service_name in ('iot','storage','api_gateway','ymq','serverless','ydb')
                    WHEN $param = 'Marketplace' then sku.service_group = 'Marketplace'
                else sku.sku_id is not null
            END 
    END DEFINE;

    DEFINE SUBQUERY $agg_fact($param,$period) AS
        SELECT 
            per,
            sum(real_consumption_vat) as real_consumption_vat
        FROM 
            $cons_fact($param,$period)
        GROUP BY 
            per
    END DEFINE;

    DEFINE SUBQUERY $cons_plan($param,$period) AS 
        SELECT 
            plan_optim,
            $round_period_date($period,`date`) as per
        FROM 
            `//home/cloud_analytics/marketing/plan_daily`
        WHERE 
            --$format_date(DateTime::StartOfYear($parse_date(`date`))) = $format_date(DateTime::StartOfYear(CurrentUtcDatetime()))
            $round_period_date('year',`date`) = $round_current_time('year')
            AND CASE
                WHEN $param = 'DataPlatform' then service in ('mdb')
                WHEN $param = 'ML_AI' then service in ('cloud_ai')
                WHEN $param = 'Kubernetes' then service in ('mk8s')
                WHEN $param = 'Serverless' then service in ('storage')
                WHEN $param = 'Marketplace' then service in ('marketplace')
                else service is not null
            END 
    END DEFINE;

    DEFINE SUBQUERY $agg_plan($param,$period) AS
        SELECT 
            sum(plan_optim) as plan_optim,
            per
        FROM    
            $cons_plan($param,$period)
        GROUP BY 
            per
    END DEFINE;

    DEFINE SUBQUERY $plan_fact($param,$period) AS
        SELECT 
            fact.per as per,
            plan_optim,
            real_consumption_vat
        FROM 
            $agg_fact($param,$period) as fact 
        JOIN 
            $agg_plan($param,$period) as plan 
        ON 
            DateTime::MakeDatetime($parse_date(fact.per)) = DateTime::MakeDatetime($parse_date(plan.per))
        ORDER BY 
            per
    END DEFINE;

    DEFINE SUBQUERY $plan_fact_running_total($param,$period) AS
        SELECT
            per,
            SUM(plan_optim) OVER w1 AS plan_optim_rt,
            SUM(real_consumption_vat) OVER w1 AS real_consumption_vat_rt,    
        FROM $plan_fact($param,$period)
        WINDOW
            w1 AS (ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
    END DEFINE;

    DEFINE SUBQUERY $res($param,$period) AS
        SELECT 
            per as `date`,
            'plan_fact_revenue'||'_'||$param  as metric_id,
            'Plan Fact Revenue Running Total'||'_'||$param  as metric_name,
            'general' as metric_group,
            'general' as metric_subgroup,
            '%' as metric_unit, 
            1 as is_ytd,
            $period as period,
            (real_consumption_vat_rt/plan_optim_rt+0.0) * 100 AS metric_value
        FROM $plan_fact_running_total($param,$period)
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth1($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'DataPlatform', $period, 'straight', 'goal', '100')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth2($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'ML_AI', $period, 'straight', 'goal', '100')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth3($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'Serverless', $period, 'straight', 'goal', '100')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth4($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'Marketplace', $period, 'straight', 'goal', '100')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth5($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'Kubernetes', $period, 'straight', 'goal', '100')
        )
    END DEFINE;

    $s1 = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth1
        );

    $s2 = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth2
        );

    $s3 = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth3
        );

    $s4 = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth4
        );        

    $s5 = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth5
        );                
        

    PROCESS $s1()
    UNION ALL
    PROCESS $s2()
    UNION ALL
    PROCESS $s3()
    UNION ALL
    PROCESS $s4()
    UNION ALL
    PROCESS $s5()
            
END DEFINE;

EXPORT $plan_fact_revenue_product;