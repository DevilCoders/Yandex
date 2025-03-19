/*___DOC___
====Plan-Fact Revenue YTD
Отношение фактической выручки к запланированной с начала года.
Для плана на H1 берется факт первого полугодия (вопрос к обсуждению).
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/plan_fact_revenue_ytd.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

PRAGMA AnsiInForEmptyOrNullableItemsCollections;

$format_name = ($name) -> { RETURN Unicode::ToLower(CAST(coalesce($name,'') as Utf8)) };

$replace = Re2::Replace(" ");

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
                        AsTuple('segment', coalesce($x[1],''))
                    )
                )
            }
        )
};

DEFINE SUBQUERY $plan_fact_revenue_ytd() AS
    DEFINE SUBQUERY $cons_fact($params) AS 
        SELECT 
            $round_period_date($params['period'],billing_record_msk_date) as per,
            -- billing_record_real_consumption_rub_vat
            billing_record_total_redistribution_rub_vat
        FROM `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption` as acube
        WHERE 
            CASE
                WHEN $params['segment'] = 'all' THEN True
                WHEN $params['segment'] = 'Enterprise' THEN crm_segment = 'Enterprise'
                WHEN $params['segment'] = 'SMB' THEN crm_segment in ('Medium','Mass')
                WHEN $params['segment'] = 'Public_sector' THEN crm_segment in ('Public sector')
                WHEN $params['segment'] = 'DataPlatform' then sku_service_group = 'Data Storage and Analytics' and sku_service_name in ('datalens','mdb')
                WHEN $params['segment'] = 'ML_AI' then sku_service_group = 'ML and AI'
                WHEN $params['segment'] = 'Serverless' then sku_service_name in ('iot','storage','api_gateway','ymq','serverless','ydb')
                WHEN $params['segment'] = 'Marketplace' then sku_service_group = 'Marketplace'
                WHEN $params['segment'] = 'Compute' then sku_service_name in ('compute')
                WHEN $params['segment'] = 'Infrastructure' then (sku_service_group = 'Infrastructure' OR sku_service_name = 'monitoring') and  billing_record_origin_service != 'mk8s'
                WHEN $params['segment'] = 'Kubernetes' then (sku_service_name = 'mk8s' or billing_record_origin_service = 'mk8s')
                WHEN $params['segment'] = 'Cloud Native' then sku_service_group = 'Cloud Native' AND sku_service_name != 'monitoring'
                ELSE $params['segment'] = sku_service_group
            END 
            AND if($params['period'] = 'year', 
                $round_period_date($params['period'],billing_record_msk_date) = $round_current_time($params['period']),
                $round_period_date($params['period'],billing_record_msk_date) < $round_current_time($params['period']))
            AND $round_period_date('year',billing_record_msk_date) = $round_current_time('year')   
    END DEFINE;

    DEFINE SUBQUERY $agg_fact($params) AS
        SELECT 
            per,
            sum(billing_record_total_redistribution_rub_vat) as billing_record_real_consumption_rub_vat
        FROM 
            $cons_fact($params)
        GROUP BY 
            per
    END DEFINE;

    DEFINE SUBQUERY $cons_plan($params) AS 
        /*SELECT 
            $round_period_date($params['period'],d_date) as per,
            plan
        FROM 
            `//home/cloud_analytics/marketing/plan2021_h2_segment`
        WHERE 1=1
            --$round_period_date('year',d_date) = $round_current_time('year')
            AND CASE
                WHEN $params['segment'] = 'all' THEN True
                WHEN $params['segment'] = 'Enterprise' THEN segment = 'Enterprise'
                WHEN $params['segment'] = 'SMB' THEN segment in ('Mass_company','Mass_indv','Medium')
                WHEN $params['segment'] = 'Public_sector' THEN segment in ('Public sector')
                ELSE null
            END 
        UNION ALL
        SELECT 
            $round_period_date($params['period'],d_date) as per,
            plan
        FROM 
            `//home/cloud_analytics/marketing/plan2021_h2_services`
        WHERE 1=1
            --$round_period_date('year',d_date) = $round_current_time('year')
            AND CASE
                WHEN $params['segment'] = 'DataPlatform' then service in ('mdb')
                WHEN $params['segment'] = 'ML_AI' then service in ('cloud_ai')
                WHEN $params['segment'] = 'Kubernetes' then service in ('mk8s')
                WHEN $params['segment'] = 'Serverless' then service in ('storage')
                WHEN $params['segment'] = 'Marketplace' then service in ('marketplace')
                WHEN $params['segment'] = 'Compute' then service in ('compute')
                else null
            END 
        UNION ALL*/
        SELECT 
            $round_period_date($params['period'],d_date) as per,
            plan
        FROM 
            `//home/cloud_analytics/analytics/prod/plans/2022/plan_final_v01`
        WHERE 
            CASE
                WHEN $params['segment'] = 'all' THEN True
                WHEN $params['segment'] = 'Enterprise' THEN segment = 'Enterprise'
                WHEN $params['segment'] = 'SMB' THEN segment in ('Medium_Mass_C','Mass_indv','Mass_switzerland')
                WHEN $params['segment'] = 'Public_sector' THEN segment in ('Public sector')
                ELSE $params['segment'] = service
            END 
            and plan_type = 'basic'
        /*UNION ALL
        SELECT 
            $round_period_date($params['period'],billing_record_msk_date) as per,
            billing_record_real_consumption_rub_vat as plan
        FROM `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption` as acube
        WHERE 
            CASE
                WHEN $params['segment'] = 'all' THEN True
                WHEN $params['segment'] = 'Enterprise' THEN crm_segment = 'Enterprise'
                WHEN $params['segment'] = 'SMB' THEN crm_segment in ('Medium','Mass')
                WHEN $params['segment'] = 'Public_sector' THEN crm_segment in ('Public sector')
                WHEN $params['segment'] = 'DataPlatform' then sku_service_group = 'Data Storage and Analytics' and sku_service_name in ('datalens','mdb')
                WHEN $params['segment'] = 'ML_AI' then sku_service_group = 'ML and AI'
                WHEN $params['segment'] = 'Serverless' then sku_service_name in ('iot','storage','api_gateway','ymq','serverless','ydb')
                WHEN $params['segment'] = 'Marketplace' then sku_service_group = 'Marketplace'
                WHEN $params['segment'] = 'Compute' then sku_service_name in ('compute')
                WHEN $params['segment'] = 'Infrastructure' then (sku_service_group = 'Infrastructure' OR sku_service_name = 'monitoring') and  billing_record_origin_service != 'mk8s'
                WHEN $params['segment'] = 'Kubernetes' then (sku_service_name = 'mk8s' or billing_record_origin_service = 'mk8s')
                WHEN $params['segment'] = 'Cloud Native' then sku_service_group = 'Cloud Native' AND sku_service_name != 'monitoring'
                ELSE $params['segment'] = sku_service_group
            END 
            AND $round_period_date('day',billing_record_msk_date) < '2021-07-01'*/
    END DEFINE;

    DEFINE SUBQUERY $agg_plan($params) AS
        SELECT 
            sum(plan) as plan,
            per
        FROM    
            $cons_plan($params)
        GROUP BY 
            per
    END DEFINE;

    DEFINE SUBQUERY $plan_fact($params) AS
        SELECT 
            fact.per as per,
            plan,
            billing_record_real_consumption_rub_vat
        FROM 
            $agg_fact($params) as fact 
        JOIN 
            $agg_plan($params) as plan 
        ON 
            DateTime::MakeDatetime($parse_date(fact.per)) = DateTime::MakeDatetime($parse_date(plan.per))
        ORDER BY 
            per
    END DEFINE;

    DEFINE SUBQUERY $plan_fact_running_total($params) AS
        SELECT
            per,
            SUM(plan) OVER w1 AS plan_rt,
            SUM(billing_record_real_consumption_rub_vat) OVER w1 AS billing_record_real_consumption_rub_vat_rt    
        FROM $plan_fact($params)
        WINDOW
            w1 AS (ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
    END DEFINE;

    DEFINE SUBQUERY $res($params) AS
        SELECT 
            per as `date`,
            'plan_fact_revenue_ytd_'||$format_name($params['segment']) as metric_id,
            'Plan Fact Revenue YTD '||CAST($params['segment'] as Utf8) as metric_name,
            'general' as metric_group,
            'general' as metric_subgroup,
            '%' as metric_unit, 
            1 as is_ytd,
            CAST($params['period'] as Utf8) as period,
            (billing_record_real_consumption_rub_vat_rt/plan_rt+0.0) * 100 AS metric_value
        FROM $plan_fact_running_total($params)
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
                        'day', 
                        'week',
                        'month', 
                        'quarter',
                        'year'
                    ), 
                    AsList(
                        'all',
                        'Enterprise',
                        'SMB',
                        'Public_sector',
                        'DataPlatform',
                        'ML_AI',
                        'Kubernetes',
                        'Serverless',
                        'Marketplace',
                        'Compute',
                        'Infrastructure',
                        'Data Storage and Analytics',
                        'Kubernetes',
                        'ML and AI',
                        'Support',
                        'Marketplace',
                        'Cloud Native',
                        'Yandex Services',
                        'Adjustments'
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $plan_fact_revenue_ytd;
