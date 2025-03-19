/*___DOC___
====New BAs (Companies) Plan-Fact
Отношение количества биллинг-аккаунтов-юрлиц (%%billing_account_type='company'%%), зарегистрировавшихся за указанный период за исключением фрода (%%billing_account_is_suspended_by_antifraud = False%%) к Плановому значению (segment in ('Mass_C','ISV','Mass_S','Mass_KZ_C')).
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/marketing/new_bas_companies_plan_fact.sql" formatter="yql"}}
}>
___DOC___*/


use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, 
                    $round_period_datetime, $round_period_date,$round_period_format,  
                    $round_current_time;
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
                        AsTuple('mode', coalesce($x[1],''))
                    )
                )
            }
        )
};



DEFINE SUBQUERY $new_bas_companies_plan_fact() AS
    DEFINE SUBQUERY $cons_fact($params) AS
    SELECT 
        $round_period_format($params['period'],event_dt) as w_date,
        hist.billing_account_id as billing_account_id
    --FROM `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption` as cons
    --LEFT JOIN `//home/cloud-dwh/data/prod/cdm/dm_events` as events
    --ON cons.billing_account_id = events.billing_account_id
    FROM `//home/cloud-dwh/data/prod/cdm/dm_ba_history` as hist
    LEFT JOIN `//home/cloud-dwh/data/prod/ods/billing/billing_accounts` as ba
    ON hist.billing_account_id = ba.billing_account_id
    WHERE 
        billing_account_type = 'company'
        and action = 'create'
        and (hist.is_isv = True or (hist.crm_segment = 'Mass' and billing_account_type = 'company'))
        and if($params['period'] = 'year', 
            $round_period_format($params['period'],event_dttm) = $round_current_time($params['period']),
            $round_period_format($params['period'],event_dttm) < $round_current_time($params['period']))
        and $round_period_format('year',event_dttm) = $round_current_time('year')
        and ba.is_suspended_by_antifraud = False
    END DEFINE;

    DEFINE SUBQUERY $agg_fact($params) AS
        SELECT 
            w_date,
            count(distinct billing_account_id) as billing_account_id
        FROM 
            $cons_fact($params)
        GROUP BY 
            w_date
    END DEFINE;

    DEFINE SUBQUERY $calendar($params) AS
        SELECT 
            `date` as d_date,
            $format_date(DateTime::StartOfMonth($parse_date(`date`))) as m_date
        FROM `//home/cloud_analytics/marketing/plan_daily`
        GROUP BY 
            `date`,
            $format_date(DateTime::StartOfMonth($parse_date(`date`)))
    END DEFINE;

    DEFINE SUBQUERY $plan($params) AS
        SELECT 
            $format_date(($parse_date(`Month`))) as plan_m_date,
            plan
        --FROM `//home/cloud_analytics/marketing/marketing_plan_clients_20_21` as p    
        FROM `//home/cloud_analytics/marketing/marketing_plan_clients_h2_21` as p   
        WHERE 
            $format_date(DateTime::StartOfYear($parse_date(`Month`))) = $format_date(DateTime::StartOfYear(CurrentUtcDatetime()))
            --and segment = 'Mass_C'
            and segment in ('Mass_C','ISV','Mass_S','Mass_KZ_C')
    END DEFINE;


    DEFINE SUBQUERY $plan_grouped($params) AS
        SELECT
            plan_m_date,
            sum(plan) as plan
        FROM 
            $plan($params)
        GROUP BY
            plan_m_date
    END DEFINE;


    DEFINE SUBQUERY $daily_plan($params) AS
        SELECT 
            min( (plan + 0.0) / DateTime::GetDayOfMonth($parse_date(d_date))) AS plan_daily,
            c.m_date AS daily_plan_m_date
        FROM 
            $plan_grouped($params) as p
        JOIN 
            $calendar($params) as c
        ON  c.m_date = p.plan_m_date
        GROUP BY 
            c.m_date
    END DEFINE;

    DEFINE SUBQUERY $pred_final_plan($params) AS
        SELECT 
            $round_period_date($params['period'],d_date) as w_date,
            plan_daily
        FROM 
            $plan_grouped($params) as p
        JOIN 
            $calendar($params) as c
            ON  
                c.m_date = p.plan_m_date
        JOIN 
            $daily_plan($params) as d 
            ON 
                d.daily_plan_m_date = p.plan_m_date
    END DEFINE;

    DEFINE SUBQUERY $final_plan($params) AS
        SELECT
            w_date,
            sum(plan_daily) as plan_daily      
        FROM $pred_final_plan($params)
        GROUP BY 
            w_date
    END DEFINE;

    DEFINE SUBQUERY $plan_fact($params) AS
        SELECT 
            fact.w_date as w_date,
            plan_daily,
            billing_account_id
        FROM 
            $agg_fact($params) as fact 
        JOIN 
            $final_plan($params) as plan 
        ON 
            DateTime::MakeDatetime($parse_date(fact.w_date)) = DateTime::MakeDatetime($parse_date(plan.w_date))
        ORDER BY 
            w_date
    END DEFINE;


    DEFINE SUBQUERY $plan_fact_running_total($params) AS
        SELECT
            w_date,
            SUM(plan_daily) OVER w1 AS plan_optim,
            SUM(billing_account_id) OVER w1 AS billing_account_id,    
            'ytd' as mode
        FROM $plan_fact($params)
        WINDOW
            w1 AS (ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
        UNION ALL 
        SELECT
            w_date,
            SUM(plan_daily) AS plan_optim,
            SUM(billing_account_id) AS billing_account_id,
            'pop' as mode
        FROM $plan_fact($params)
        GROUP BY 
            w_date
    END DEFINE;


    DEFINE SUBQUERY $res($params) AS
        SELECT 
            w_date as `date`,
            'new_ba_companies_plan_fact_'||$format_name($params['mode']) as metric_id,
            'New BAs (Companies) Plan-Fact '||CAST($params['mode'] as Utf8) as metric_name,
            'marketing' as metric_group,
            'marketing' as metric_subgroup,
            '%' as metric_unit, 
            if(CAST($params['mode'] as Utf8) = 'ytd', 1, 0) as is_ytd,
            CAST($params['period'] as Utf8) as period,
            billing_account_id/(plan_optim+0.0) * 100 as metric_value
        FROM $plan_fact_running_total($params)
        WHERE mode = $params['mode']
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT * FROM 
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
                        'ytd',
                        'pop'
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $new_bas_companies_plan_fact;