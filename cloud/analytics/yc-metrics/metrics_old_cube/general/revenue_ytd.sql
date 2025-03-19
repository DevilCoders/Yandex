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


$format_name = ($name) -> { RETURN Unicode::ToLower(CAST(coalesce($name,'') as Utf8)) };

$replace = Re2::Replace(" ");

$cum_sum_script = @@
import numpy as np
def cumsum(a):
    return list(np.cumsum(list(a)))
    @@;
$cumsum = Python3::cumsum(
    Callable<(List<Double>)->List<Double>>,
    $cum_sum_script
);




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



DEFINE SUBQUERY $revenue_ytd() AS 
    

    DEFINE SUBQUERY $revenue_raw($params) AS
        SELECT 
            `date`,
            sum(real_consumption_vat) as paid_cons
        FROM (
            SELECT 
                $round_period_datetime($params['period'],event_time) as `date`,
                real_consumption_vat
            FROM 
                `//home/cloud_analytics/cubes/acquisition_cube/cube`
            WHERE 
                event = 'day_use'
                and if($params['period'] = 'year', 
                    $round_period_datetime($params['period'],event_time) = $round_current_time($params['period']),
                    $round_period_datetime($params['period'],event_time) < $round_current_time($params['period']))
                and $round_period_datetime('year',event_time) = $round_current_time('year')
                AND 
                    CASE 
                        WHEN $params['segment'] = 'all' THEN True
                        WHEN $params['segment'] = 'Enterprise' THEN crm_account_segment = 'Enterprise'
                        WHEN $params['segment'] = 'SMB' THEN crm_account_segment in ('Mass', 'Medium')
                        WHEN $params['segment'] = 'Mass' THEN crm_account_segment in ('Mass')
                        WHEN $params['segment'] = 'Meduim' THEN crm_account_segment in ('Medium')
                        WHEN $params['segment'] = 'Public sector' THEN crm_account_segment  = 'Public sector'
                        ELSE True
                    END 
        )
        GROUP BY `date`
        ORDER BY `date`
        --LIMIT 1000000000000000000 
    END DEFINE;

    DEFINE SUBQUERY $res($params) AS
        SELECT 
            a.0 as `date`,
            'revenue_ytd_'||$format_name($params['segment']) as metric_id,
            'Revenue YTD '||CAST($params['segment'] as Utf8) as metric_name,
            'general' as metric_group,
            'money' as metric_subgroup,
            '₽' as metric_unit, 
            1 as is_ytd,
            CAST($params['period'] AS Utf8) as period,
            a.1 as metric_value
        FROM ( 
            SELECT
                ListZip(
                    AGGREGATE_LIST(`date`),
                    $cumsum(AGGREGATE_LIST(paid_cons))
                ) as a
            FROM 
                $revenue_raw($params)
        ) 
        FLATTEN BY a 
    END DEFINE;


DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $params, 'straight', 'growth', '')
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
                        'Medium',
                        'Mass',
                        'SMB',
                        'Public sector'
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()


END DEFINE;

EXPORT $revenue_ytd;
