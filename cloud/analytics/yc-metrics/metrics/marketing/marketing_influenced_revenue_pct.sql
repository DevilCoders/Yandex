/*___DOC___
==== Marketing Influenced Revenue pct

Метрика отражает, какая доля выручки выручки приходится на Маркетинг %%marketing_attribution_channel_marketing_influenced = 'Marketing'%%.
То, по каким правилам опеределяется, что marketing_attribution_channel_marketing_influenced = 'Marketing' - добавлю информацию чуть позже.


<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/marketing/marketing_influenced_revenue_pct.sql" formatter="yql"}}
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
                        AsTuple('segment', coalesce($x[1],''))
                    )
                )
            }
        )
};




DEFINE SUBQUERY $marketing_influenced_revenue_pct() AS
    DEFINE SUBQUERY $res($params) AS 
        SELECT 
            `date`,
            'mrkt_influenced_rev_pct_' ||$format_name($params['segment'])  as metric_id,
            'Marketing Influenced Revenue Pct '||cast($params['segment'] as Utf8) as metric_name,
            'marketing' as metric_group,
            'marketing' as metric_subgroup,
            '%' as metric_unit,
            0 as is_ytd,
            cast($params['period'] as Utf8) as period,
            SUM_IF(real_consumption_vat * marketing_attribution_event_exp_7d_half_life_time_decay_weight, marketing_attribution_channel_marketing_influenced = 'Marketing') / SUM(real_consumption_vat * marketing_attribution_event_exp_7d_half_life_time_decay_weight) *100 as metric_value
        FROM (
        SELECT 
            $round_period_datetime($params['period'], event_time) as `date`,
            real_consumption_vat,
            marketing_attribution_event_exp_7d_half_life_time_decay_weight,
            marketing_attribution_channel_marketing_influenced
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube_with_marketing_attribution`
        WHERE event = 'day_use'
            AND $round_period_datetime($params['period'],event_time) < $round_current_time($params['period'])
            AND CASE
                WHEN $params['segment'] = 'Enterprise' THEN crm_account_segment in ('Enterprise')
                WHEN $params['segment'] = 'SMB' THEN crm_account_segment in ('Mass', 'Medium')
                ELSE True
            END 
        )
        GROUP BY `date`
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $params, 'straight', 'standart', '')
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
                        'SMB',
                        'Enterprise'
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()


END DEFINE;

EXPORT $marketing_influenced_revenue_pct;