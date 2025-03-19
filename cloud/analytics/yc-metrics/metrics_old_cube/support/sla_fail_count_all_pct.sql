/*___DOC___
==== SLA Fail Count ALL pct
Доля комментариев, созданных за неделю и по которым нарушен SLA
Метрика считается суммарно для всех видов поддержки
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/sla_fail_count_all_pct.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time, $round_period_format;
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



DEFINE SUBQUERY $sla_fail_count_all_pct() AS
    DEFINE SUBQUERY $res($params) AS
        SELECT 
            `date`,
            'support_sla_fails_count_pct_'||$format_name($params['mode']) as metric_id,
            'Support - SLA Fails Count (Pct) '||$format_name($params['mode']) as metric_name,
            'support' as metric_group,
            'support' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            cast($params['period'] as Utf8) as period,
            sum(sla_failed+0.0) / count(startrek_id) * 100 as metric_value
        FROM(
        SELECT
                comments.startrek_id as startrek_id,
                --$round_period_datetime($period,comments.created_at) as `date`,
                $round_period_format($params['period'],comments.created_at) as `date`,
                sla_failed,
                --billing_id,
                pay
            FROM (SELECT * FROM `//home/cloud_analytics/import/yc-support/static/cloudsupport_comments`) as comments
            LEFT JOIN (SELECT * FROM `//home/cloud_analytics/import/yc-support/static/cloudsupport_issues`) as issues
            ON comments.issue_key = issues.startrek_key
            WHERE 
                --sla_failed = 1 
                case
                    when $params['mode'] = 'all' then True
                    when $params['mode'] = 'allpaid' then pay in ('standard','business','premium')
                    else pay = $params['mode']
                END -- AND issue_key = 'CLOUDSUPPORT-70756'
                AND $round_period_format('day',comments.created_at) < '2021-08-01'
            UNION ALL
            SELECT
                sla.issue_id as startrek_id,
                --$round_period_format($params['period'],CAST(substring(Yson::ConvertToString(started_at),0,10) as Date)) as `date`,
                $round_period_format($params['period'],started_at) as `date`,
                if(spent_ms>fail_threshold_ms,1,0) as sla_failed,
                sla.payment_tariff as pay
            FROM 
                `//home/cloud-dwh/data/preprod/cdm/support/dm_yc_support_issues`  as issues
            JOIN 
                --`//home/cloud-dwh/data/preprod/ods/startrek/cloud_support/issue_slas`  as sla
                `//home/cloud-dwh/data/preprod/cdm/support/dm_yc_support_comment_slas` as sla
            ON 
                issues.issue_id = sla.issue_id
            WHERE 
                sla.timer_id = 3052
                --and substring(Yson::ConvertToString(started_at),0,10) >= '2021-08-01'
                and $round_period_format('day',started_at) >= '2021-08-01'
                and sla.spent_ms IS NOT NULL
                --and spent_ms > fail_threshold_ms
                and case
                    when $params['mode'] = 'all' then True
                    when $params['mode'] = 'allpaid' then sla.payment_tariff  in ('standard','business','premium') --pay
                    when $params['mode'] = 'free' then sla.payment_tariff  not in ('standard','business','premium') --pay
                    else sla.payment_tariff = $params['mode'] --pay
                END
        )
        WHERE 1=1
            --`date` < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
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

EXPORT $sla_fail_count_all_pct;