/*___DOC___
====Sustainable Revenue
Устойчивая выручка - выручка за неделю без ряда sku, потребление по которым начисляется в конце месяца
<{График
{{iframe frameborder="0" width="70%" height="300px" src="https://charts.yandex-team.ru/preview/editor/hv5twavvj2jqb?metric_name=Sustainable%20Revenue&6112cac5-7432-4748-a900-99da3f669075=Sustainable%20Revenue&metricName=Sustainable%20Revenue&_embedded=1"}}
}>
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/sustainable_revenue.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;

IMPORT time_series SYMBOLS $metric_growth;
$cum_sum_script = @@
import numpy as np
def cumsum(a):
    return list(np.cumsum(list(a)))
    @@;
$cumsum = Python3::cumsum(
    Callable<(List<Double>)->List<Double>>,
    $cum_sum_script
);
DEFINE SUBQUERY $revenue_cum_2021() AS 
    

    DEFINE SUBQUERY $revenue_raw($params,$period) AS
        SELECT 
            `date`,
            sum(real_consumption_vat) as paid_cons
        FROM (
            SELECT 
                --$format_date(DateTime::StartOfWeek($parse_datetime(event_time))) as `date`,
                $round_period_datetime($period,event_time) as `date`,
                real_consumption_vat
            FROM 
                `//home/cloud_analytics/cubes/acquisition_cube/cube`
            WHERE 
                event = 'day_use'
                --and $format_date(DateTime::StartOfWeek($parse_datetime(event_time))) < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
                and if($period = 'year', $round_period_datetime($period,event_time) = $round_current_time($period),
                    $round_period_datetime($period,event_time) < $round_current_time($period))
                and $round_period_datetime('year',event_time) = $round_current_time('year')
                --and $format_date($parse_datetime(event_time)) >= '2021-01-01'
                AND CASE $params['filter_field'][0]
                    -- WHEN 'no_filter' THEN 'no_filter'
                    WHEN 'crm_account_segment' THEN crm_account_segment
                    ELSE 'no_filter'
                END in $params['filter_values']
        )
        GROUP BY `date`
        ORDER BY `date`
        LIMIT 1000000000000000000 
    END DEFINE;

    DEFINE SUBQUERY $res($params,$period) AS
        SELECT 
            a.0 as `date`,
            $params['metric_id'][0] as metric_id,
            $params['metric_name'][0] as metric_name,
            'general' as metric_group,
            'money' as metric_subgroup,
            '₽' as metric_unit, 
            1 as is_ytd,
            $period as period,
           a.1 as metric_value
        FROM ( 
            SELECT
                ListZip(
                    AGGREGATE_LIST(`date`),
                    $cumsum(AGGREGATE_LIST(paid_cons))
                ) as a
            FROM 
                $revenue_raw($params,$period)
        ) 
        FLATTEN BY a 
    END DEFINE;


    DEFINE SUBQUERY $res_with_metric_growth1($period) AS (
        SELECT *
        FROM 
            $metric_growth(
                $res,
                AsDict(
                    AsTuple(
                        'metric_id',
                        AsList('revenue_cum_2021_all')
                    ),
                    AsTuple(
                        'metric_name',
                        AsList('YC Revenue Cum (All)')
                    ),
                    AsTuple(
                        'filter_field',
                        AsList('no_filter')
                    ),
                    AsTuple(
                        'filter_values',
                        AsList('no_filter')
                    )
                ), 
                $period, 'straight','growth', ''
        )
    )
    END DEFINE;


    DEFINE SUBQUERY $res_with_metric_growth2($period) AS (
        SELECT *
        FROM
            $metric_growth(
                $res,
                AsDict(
                    AsTuple(
                        'metric_id',
                        AsList('revenue_cum_2021_enterprise')
                    ),
                    AsTuple(
                        'metric_name',
                        AsList('YC Revenue Cum (Enterprise)')
                    ),
                    AsTuple(
                        'filter_field',
                        AsList('crm_account_segment')
                    ),
                    AsTuple(
                        'filter_values',
                        AsList('Enterprise')
                    )
            ),
            $period,'straight','growth', ''
        )
    )
    END DEFINE;


    DEFINE SUBQUERY $res_with_metric_growth3($period) AS (

        SELECT *
        FROM 
            $metric_growth(
                $res,
                AsDict(
                    AsTuple(
                        'metric_id',
                        AsList('revenue_cum_2021_smb')
                    ),
                    AsTuple(
                        'metric_name',
                        AsList('YC Revenue Cum (SMB)')
                    ),
                    AsTuple(
                        'filter_field',
                        AsList('crm_account_segment')
                    ),
                    AsTuple(
                        'filter_values',
                        AsList('Mass', 'Medium')
                    )
            ),
            $period,'straight','growth', ''
        )
    )
    END DEFINE;



    DEFINE SUBQUERY $res_with_metric_growth4($period) AS (

        SELECT *
        FROM 
            $metric_growth(
                $res,
                AsDict(
                    AsTuple(
                        'metric_id',
                        AsList('revenue_cum_2021_gov')
                    ),
                    AsTuple(
                        'metric_name',
                        AsList('YC Revenue Cum (Government)')
                    ),
                    AsTuple(
                        'filter_field',
                        AsList('crm_account_segment')
                    ),
                    AsTuple(
                        'filter_values',
                        AsList('Public sector')
                    )
            ),
            $period,'straight','growth', ''
        )
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

    PROCESS $s1()
    
    UNION ALL 

    PROCESS $s2()

    UNION ALL

    PROCESS $s3()

    UNION ALL

    PROCESS $s4()


END DEFINE;

EXPORT $revenue_cum_2021;
