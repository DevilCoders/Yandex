/*___DOC___
====Plan-Fact Revenue Enterprise WoW
Отношение фактической выручки к запланированной для сегменета Enterprise по неделям.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/eterprise/plan_fact_revenue_ent_wow.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;


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

DEFINE SUBQUERY $plan_fact_revenue_pop() AS
    DEFINE SUBQUERY $cons_fact($params) AS 
        SELECT 
            $round_period_datetime($params['period'],event_time) as per,
            real_consumption_vat
        FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 
            event = 'day_use'
            AND CASE
                WHEN $params['segment'] = 'all' THEN True
                WHEN $params['segment'] = 'Enterprise' THEN crm_account_segment = 'Enterprise'
                WHEN $params['segment'] = 'SMB' THEN crm_account_segment in ('Medium','Mass')
                WHEN $params['segment'] = 'Public_sector' THEN crm_account_segment in ('Public sector')
                ELSE True
            END 
            AND if($params['period'] = 'year', 
                $round_period_datetime($params['period'],event_time) = $round_current_time($params['period']),
                $round_period_datetime($params['period'],event_time) < $round_current_time($params['period']))
            AND $round_period_datetime('year',event_time) = $round_current_time('year')
    END DEFINE;

    DEFINE SUBQUERY $agg_fact($params) AS 
        SELECT 
            per,
            sum(real_consumption_vat) as real_consumption_vat
        FROM 
            $cons_fact($params)
        GROUP BY 
            per
    END DEFINE;

    DEFINE SUBQUERY $cons_plan($params) AS 
        SELECT 
            $round_period_date($params['period'],d_date) as per,
            plan
        FROM 
            `//home/cloud_analytics/marketing/plan2021_h2_segment`
        WHERE 
            $round_period_date('year',d_date) = $round_current_time('year')
            AND CASE
                WHEN $params['segment'] = 'all' THEN True
                WHEN $params['segment'] = 'Enterprise' THEN segment = 'Enterprise'
                WHEN $params['segment'] = 'SMB' THEN segment in ('Mass_company','Mass_indv','Medium')
                WHEN $params['segment'] = 'Public_sector' THEN segment in ('Public sector')
                ELSE True
            END 
        UNION ALL
        SELECT 
            $round_period_datetime($params['period'],event_time) as per,
            real_consumption_vat as plan
        FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 
            event = 'day_use'
            AND CASE
                WHEN $params['segment'] = 'all' THEN True
                WHEN $params['segment'] = 'Enterprise' THEN crm_account_segment = 'Enterprise'
                WHEN $params['segment'] = 'SMB' THEN crm_account_segment in ('Medium','Mass')
                WHEN $params['segment'] = 'Public_sector' THEN crm_account_segment in ('Public sector')
                ELSE True
            END 
            AND $round_period_datetime('day',event_time) < '2021-07-01'
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
            real_consumption_vat
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
            SUM(plan) AS plan,
            SUM(real_consumption_vat) AS real_consumption_vat    
        FROM $plan_fact($params)
        GROUP BY 
            per
    END DEFINE;

    DEFINE SUBQUERY $res($params) AS
        SELECT 
            per as `date`,
            'plan_fact_revenue_pop_'||$format_name($params['segment']) as metric_id,
            'Plan Fact Revenue PoP '||CAST($params['segment'] as Utf8) as metric_name,
            'general' as metric_group,
            'general' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            CAST($params['period'] as Utf8) as period,
            (real_consumption_vat/plan+0.0) * 100 AS metric_value
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
                        'Public_sector'
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $plan_fact_revenue_pop;