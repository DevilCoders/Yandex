/*___DOC___
====Ticket rated [1|2|3]/5
Метрика выводит долю тикетов, у которых хотя бы одна из оценок меньше 4, относительно общего числа оцененных тикетов.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/number_of_tickets_rated_one_all.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time,$round_period_format;
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


DEFINE SUBQUERY $number_of_tickets_rated_one_all() AS 
    DEFINE SUBQUERY $res($params) AS
        SELECT
            `date`,
            'number_of_tickets_rated_one_'||$format_name($params['mode'])  as metric_id,
            'Number Of Tickets Rated One '||$format_name($params['mode']) as metric_name,
            'support' as metric_group,
            'support' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            cast($params['period'] as Utf8) as period,
            sum(
                if(
                    min_of(feedback_general,feedback_reaction_speed,feedback_response_completeness) < 4,
                    1.0,
                    0.0
                )
            )/count(issue_key) * 100.0 AS metric_value
        FROM (
            SELECT
                $round_period_format($params['period'],resolved_at) as `date`,
                issue_key,
                feedback_general,
                feedback_reaction_speed,
                feedback_response_completeness
            FROM
                `//home/cloud-dwh/data/prod/cdm/support/dm_yc_support_issues`
            WHERE 
                `feedback_general` not Null
                and CASE
                    when $params['mode'] = 'all' then True
                    when $params['mode'] = 'allpaid' then payment_tariff  in ('standard','business','premium')
                    when $params['mode'] = 'free' then payment_tariff  not in ('standard','business','premium')
                    else payment_tariff = $params['mode'] 
                END
                and $round_period_format($params['period'],resolved_at) < $round_current_time($params['period'])
        )
        GROUP BY `date`
    END DEFINE;


    DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $params, 'inverse', 'standart', '')
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
                        'free',
                        'allpaid',
                        'standard',
                        'business', 
                        'premium'
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()


END DEFINE;

EXPORT $number_of_tickets_rated_one_all;