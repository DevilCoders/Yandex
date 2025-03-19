/*___DOC___
====Count of day2day closed tickets
Кол-во тикетов, закрытых в день создания. В разбивке на платную и бесплатную поддержку. Даты приведены к UTC.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/count_of_closed_day2day_tickets.sql" formatter="yql"}}
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


DEFINE SUBQUERY $count_of_closed_day2day_tickets() AS
    DEFINE SUBQUERY $cnt_tickets($params) AS 
        SELECT
            $round_period_format($params['period'],created_at) as `date`,
            if(cast(created_at as Date) = cast(resolved_at as Date),1,0) as d2d,
            issue_key
        FROM
            `//home/cloud-dwh/data/prod/cdm/support/dm_yc_support_issues`
        WHERE 
            case
                when $params['mode'] = 'allpaid' then payment_tariff  in ('standard','business','premium') --pay
                when $params['mode'] = 'free' then payment_tariff  not in ('standard','business','premium') --pay
                else True
            END
            and components_quotas = False
    END DEFINE;

    DEFINE SUBQUERY $res($params) AS
        SELECT 
            `date`,
            'count_of_closed_day2day_tickets_'||$format_name($params['mode']) as metric_id,
            'count_of_closed_day2day_tickets_ '||$format_name($params['mode']) as metric_name,
            'support' as metric_group,
            'support' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            cast($params['period'] as Utf8) as period,
            Math::Round(sum(d2d)/(count(issue_key)+0.0)*100,-2) as metric_value
        FROM $cnt_tickets($params)
        WHERE
            if($params['period'] = 'year', `date` <= $round_current_time($params['period']),
                `date` < $round_current_time($params['period']))
        GROUP BY
            `date`
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
                        'day', 
                        'week',
                        'month', 
                        'quarter',
                        'year'
                    ), 
                    AsList(
                        'free',
                        'allpaid'
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $count_of_closed_day2day_tickets;
