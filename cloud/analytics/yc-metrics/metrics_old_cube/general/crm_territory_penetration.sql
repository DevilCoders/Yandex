/*___DOC___
====CRM Territory Penetration
Скользящее окно отношения числа Billing Account, с которыми была коммуникация от команды допродаж за последние 90 дней  к числу всех Billind Account сегмента Mass с платным ненулевым потреблением за этот же период.

<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/SMB/crm_territory_penetration.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time, $round_period_format;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $crm_territory_penetration() AS
    DEFINE SUBQUERY $all_ba($params) AS
        SELECT 
            --$format_date(DateTime::StartOfWeek($parse_datetime(event_time))) as w_date,
            $round_period_datetime($params,event_time) as w_date,
            billing_account_id 
        FROM  `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE event = 'day_use'
            AND ba_person_type_actual = 'company'
            AND ba_usage_status_actual = 'paid'
            AND crm_account_segment_current = 'Mass'   
            AND real_consumption > 0 
            AND is_var != 'var'
    END DEFINE;

    DEFINE SUBQUERY $list_ba($params) as 
    select
        w_date,
        AGGREGATE_LIST_DISTINCT(billing_account_id) as ba_list
    from $all_ba($params) 
    group by 
        w_date    
    END DEFINE;

    DEFINE SUBQUERY $ba_list_roll_window($params)  AS 
    SELECT
        w_date,
        IF($params = 'week',
            ListLength(ListUniq(ListFlatMap(AGGREGATE_LIST(ba_list) OVER (ORDER BY w_date ROWS BETWEEN 13 PRECEDING AND CURRENT ROW), ($x)->($x)))),
            ListLength(ListUniq(ListFlatMap(AGGREGATE_LIST(ba_list) OVER (ORDER BY w_date ROWS BETWEEN 3 PRECEDING AND CURRENT ROW), ($x)->($x))))
        ) as cnt_ba_all
    FROM $list_ba($params) 
    ORDER BY 
        w_date desc
    END DEFINE;


    DEFINE SUBQUERY $calls($params)  AS
    SELECT
        --$format_date(DateTime::StartOfWeek(DateTime::FromSeconds(CAST(call_date_start AS UInt32)))) as w_date,
        $round_period_format($params, DateTime::FromSeconds(CAST(call_date_start AS UInt32))) as w_date,
        coalesce(acc_ba_id,lead_ba_id,opp_acc_ba_id) as ba_list
    FROM `//home/cloud_analytics/kulaga/calls_cube` 
    WHERE call_user_name in ('dmtroe','gingerkote','szanozin','moiseeva-m','kvmorgachev','nikitagrekhov','sosnovskikh')
        AND call_status = 'Held'
        AND coalesce(acc_ba_id,lead_ba_id,opp_acc_ba_id) IS NOT NULL
    END DEFINE;

    DEFINE SUBQUERY $filtered_calls($params)  AS
    SELECT 
        *
    FROM $calls($params)  AS calls
    JOIN (SELECT DISTINCT billing_account_id FROM $all_ba($params)) AS filtered_ba 
            ON calls.ba_list = filtered_ba.billing_account_id
    END DEFINE;

    DEFINE SUBQUERY $agg_ba_list_calls($params)  AS
    SELECT
        w_date,
        AGGREGATE_LIST_DISTINCT(ba_list) as ba_list
    FROM $filtered_calls($params) 
    GROUP BY 
        w_date
    END DEFINE;

    DEFINE SUBQUERY $calls_ba_list_roll_window($params)  AS
    SELECT
        w_date,
        --ListLength(ListUniq(ListFlatMap(AGGREGATE_LIST(ba_list) OVER (ORDER BY w_date ROWS BETWEEN 13 PRECEDING AND CURRENT ROW), ($x)->($x)))) as cnt_ba_calls 
        IF($params = 'week',
            ListLength(ListUniq(ListFlatMap(AGGREGATE_LIST(ba_list) OVER (ORDER BY w_date ROWS BETWEEN 13 PRECEDING AND CURRENT ROW), ($x)->($x)))),
            ListLength(ListUniq(ListFlatMap(AGGREGATE_LIST(ba_list) OVER (ORDER BY w_date ROWS BETWEEN 3 PRECEDING AND CURRENT ROW), ($x)->($x))))
        ) as cnt_ba_calls
    FROM $agg_ba_list_calls($params) 
    ORDER BY 
        w_date desc
    END DEFINE;

    DEFINE SUBQUERY $res($params) AS
        SELECT 
            ba_calls.w_date AS `date`,
            'crm_territory_penetration' AS metric_id,
            'CRM Territory Penetration' AS metric_name,
            'SMB' as metric_group,
            'SMB' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            $params as period,
            CAST(cnt_ba_calls AS Double)/CAST(cnt_ba_all AS Double)*100 as metric_value
        FROM $ba_list_roll_window($params)  as ba_all 
        JOIN $calls_ba_list_roll_window($params)  as ba_calls 
            USING(w_date)
        ORDER BY 
            `date` desc
        END DEFINE;


    DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $params, 'straight', 'standart', '')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
            ['week','month'],
            $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $crm_territory_penetration;