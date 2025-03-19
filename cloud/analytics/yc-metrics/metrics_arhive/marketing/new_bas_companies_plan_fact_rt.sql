/*___DOC___
====New BAs (Companies) Plan-Fact Year-to-Day
Отношение количества биллинг-аккаунтов-юрлиц (%%ba_person_type='company'%%), зарегистрировавшихся за период с начала года по соответствующую неделю за исключением фрода (%%is_fraud = 0%%) к Плановому значению (segment = 'Mass_C') с начала года к соответствующей неделе.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/marketing/new_bas_companies_plan_fact_rt.sql" formatter="yql"}}
}>
___DOC___*/


use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $new_bas_companies_plan_fact_rt() AS
    DEFINE SUBQUERY $cons_fact($period) AS
    SELECT 
        $round_period_datetime($period,event_time) as w_date,
        billing_account_id
    FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
    WHERE 
        event = 'ba_created'
        and is_fraud = 0
        and ba_person_type = 'company'
        --AND $format_date(DateTime::StartOfWeek($parse_datetime(event_time))) < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
        --AND $format_date(DateTime::StartOfYear($parse_datetime(event_time))) = $format_date(DateTime::StartOfYear(CurrentUtcDatetime()))
        and if($period = 'year', $round_period_datetime($period,event_time) = $round_current_time($period),
            $round_period_datetime($period,event_time) < $round_current_time($period))
        and $round_period_datetime('year',event_time) = $round_current_time('year')
    END DEFINE;

    DEFINE SUBQUERY $agg_fact($period) AS
        SELECT 
            w_date,
            count(distinct billing_account_id) as billing_account_id
        FROM 
            $cons_fact($period)
        GROUP BY 
            w_date
    END DEFINE;

    DEFINE SUBQUERY $calendar($period) AS
        SELECT 
            `date` as d_date,
            $format_date(DateTime::StartOfMonth($parse_date(`date`))) as m_date
        FROM `//home/cloud_analytics/marketing/plan_daily`
        GROUP BY 
            `date`,
            $format_date(DateTime::StartOfMonth($parse_date(`date`)))
    END DEFINE;

    DEFINE SUBQUERY $plan($period) AS
        SELECT 
            $format_date(($parse_date(`Month`))) as plan_m_date,
            plan
        FROM `//home/cloud_analytics/marketing/marketing_plan_clients_20_21` as p    
        WHERE 
            $format_date(DateTime::StartOfYear($parse_date(`Month`))) = $format_date(DateTime::StartOfYear(CurrentUtcDatetime()))
            and segment = 'Mass_C'
    END DEFINE;

    DEFINE SUBQUERY $daily_plan($period) AS
        SELECT 
            min( (plan + 0.0) / DateTime::GetDayOfMonth($parse_date(d_date))) AS plan_daily,
            c.m_date AS daily_plan_m_date
        FROM 
            $plan($period) as p
        JOIN 
            $calendar($period) as c
        ON  c.m_date = p.plan_m_date
        GROUP BY 
            c.m_date
    END DEFINE;

    DEFINE SUBQUERY $pred_final_plan($period) AS
        SELECT 
            --$format_date(DateTime::StartOfWeek($parse_date(d_date))) as w_date,
            $round_period_date($period,d_date) as w_date,
            plan_daily
        FROM 
            $plan($period) as p
        JOIN 
            $calendar($period) as c
            ON  
                c.m_date = p.plan_m_date
        JOIN 
            $daily_plan($period) as d 
            ON 
                d.daily_plan_m_date = p.plan_m_date
    END DEFINE;

    DEFINE SUBQUERY $final_plan($period) AS
        SELECT
            w_date,
            sum(plan_daily) as plan_daily      
        FROM $pred_final_plan($period)
        GROUP BY 
            w_date
    END DEFINE;

    DEFINE SUBQUERY $plan_fact($period) AS
        SELECT 
            fact.w_date as w_date,
            plan_daily,
            billing_account_id
        FROM 
            $agg_fact($period) as fact 
        JOIN 
            $final_plan($period) as plan 
        ON 
            DateTime::MakeDatetime($parse_date(fact.w_date)) = DateTime::MakeDatetime($parse_date(plan.w_date))
        ORDER BY 
            w_date
    END DEFINE;


    DEFINE SUBQUERY $plan_fact_running_total($period) AS
        SELECT
            w_date,
            SUM(plan_daily) OVER w1 AS plan_optim,
            SUM(billing_account_id) OVER w1 AS billing_account_id,    
        FROM $plan_fact($period)
        WINDOW
            w1 AS (ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
    END DEFINE;


    DEFINE SUBQUERY $res($param,$period) AS
        SELECT 
            w_date as `date`,
            'new_ba_companies_plan_fact_rt' as metric_id,
            'New BAs (Companies) Plan-Fact Year-to-day' as metric_name,
            'marketing' as metric_group,
            'marketing' as metric_subgroup,
            '%' as metric_unit, 
            1 as is_ytd,
            $period as period,
            billing_account_id/(plan_optim+0.0) * 100 as metric_value
        FROM $plan_fact_running_total($period)
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth($period) AS (
        SELECT * FROM 
            $metric_growth($res, '',$period, 'straight', 'goal', '100')
    )
    END DEFINE;

    $s = SubqueryUnionAllFor(
        ['day','week','month','quarter','year'],
        $res_with_metric_growth
    );


    PROCESS $s()

END DEFINE;

EXPORT $new_bas_companies_plan_fact_rt;