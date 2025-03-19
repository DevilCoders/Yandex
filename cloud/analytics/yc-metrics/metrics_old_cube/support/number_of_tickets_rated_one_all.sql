/*___DOC___
====Number of tickets rated one all
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
            -- "issue_key",
            -- "completely",
            --feedback.`date` as `date`, 
            `date`,
            'number_of_tickets_rated_one_'||$format_name($params['mode'])  as metric_id,
            'Number Of Tickets Rated One '||$format_name($params['mode']) as metric_name,
            'support' as metric_group,
            'support' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            cast($params['period'] as Utf8) as period,
            --sum(if(min_of(rapid,general,completely)=1,1.0,0.0))/count(issue_key) * 100.0 AS metric_value
            sum(if(min_of(feedback_general,feedback_reaction_speed,feedback_response_completeness)=1,1.0,0.0))/count(issue_key) * 100.0 AS metric_value
            -- "description",
            -- "general",
            -- "rapid"
        /*FROM (
            SELECT
                --$format_date(DateTime::StartOfWeek(created_at)) as `date`,
                --$round_period_datetime($period,created_at) as `date`,
                $round_period_format($params['period'],created_at) as `date`,
                issue_key,
                rapid,
                general,
                completely
            FROM `//home/cloud_analytics/import/yc-support/static/cloudsupport_feedback`
        ) as feedback
        LEFT JOIN (
            SELECT 
                startrek_key,
                pay 
            FROM `//home/cloud_analytics/import/yc-support/static/cloudsupport_issues`
        ) as issues
        ON feedback.issue_key = issues.startrek_key
        WHERE 
            case
                when $params['mode'] = 'all' then True
                when $params['mode'] = 'allpaid' then pay in ('standard','business','premium')
                else pay = $params['mode']
            END
            and `date` < $round_current_time($params['period'])  --$format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
        */
        FROM (
        SELECT
            $round_period_format($params['period'],resolved_at) as `date`,
            issue_key,
            feedback_general,
            feedback_reaction_speed,
            feedback_response_completeness
        FROM
            `//home/cloud-dwh/data/preprod/cdm/support/dm_yc_support_issues`
        WHERE 
            `feedback_general` not Null
            and CASE
                when $params['mode'] = 'all' then True
                when $params['mode'] = 'allpaid' then payment_tariff  in ('standard','business','premium') --pay
                when $params['mode'] = 'free' then payment_tariff  not in ('standard','business','premium') --pay
                else payment_tariff = $params['mode'] --pay
            END
            and $round_period_format($params['period'],resolved_at) < $round_current_time($params['period'])  --$format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
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