/*___DOC___
====СSAT
Customer Satisfaction. Считается как процент итоговых оценок 4 и 5 от кол-ва всех оценок, поставленных за неделю. Под итоговой оценкой понимается **минимальная** оценка из %%general%%, %%rapid%% и %%completely%%
Метрика считается отдельно для каждого из 4 видов поддержки: %%premium%%, %%business%%, %%standard%% и %%free%%

<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/customer_satisfaction.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

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
                        AsTuple('support_type', coalesce($x[0],'')),
                        AsTuple('period', coalesce($x[1],''))
                    )
                )
            }
        )
};


DEFINE SUBQUERY $customer_satisfaction() AS 
    DEFINE SUBQUERY $res($support_type,$period) AS
        SELECT
            -- "issue_key",
            -- "completely",
            feedback.`date` as `date`, 
            'supporc_csat_'||$support_type as metric_id,
            'Support - CSAT (' || $support_type || ')' as metric_name,
            'support' as metric_group,
            'support' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            $period as period,
            sum(if(min_of(rapid,general,completely)>=4,1.0,0.0))/count(issue_key) * 100.0 AS metric_value
            -- "description",
            -- "general",
            -- "rapid"
        FROM (
            SELECT
                --$format_date(DateTime::StartOfWeek(created_at)) as `date`,
                $round_period_date($period,created_at) as `date`,
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
            pay = $support_type
            and `date` < $round_current_time($period)-- $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
        GROUP BY feedback.`date`
    END DEFINE;

/*
    SELECT * FROM
    $metric_growth($res, 'free', 'inverse','standart','')

    UNION ALL

    SELECT * FROM
    $metric_growth($res, 'standard', 'inverse','standart','')

    UNION ALL

    SELECT * FROM
    $metric_growth($res, 'business', 'inverse','standart','')

    UNION ALL
    
    SELECT * FROM
    $metric_growth($res, 'premium', 'inverse','standart','')
*/
    DEFINE SUBQUERY $res_with_metric_growth($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $param, $period, 'inverse', 'standart', '')
        )
    END DEFINE;

        $s = SubqueryUnionAllFor(
        $make_param_dict(
            $product(
                AsList(
                    AsList(
                        'free', 
                        'standard',
                        'business', 
                        'premium'
                    ), 
                    AsList(
                        'day',
                        'week',
                        'month',
                        'quarter',
                        'year'
                    )
                )
            )
        ),
        $res_with_metric_growth
    );

    PROCESS $s();

END DEFINE;

EXPORT $customer_satisfaction;