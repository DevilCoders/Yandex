/*___DOC___
====Active Users 
Число активных пользователей по неделям. Активным пользователем считается Billing Account, по которому было потребление в течение недели.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/active_users.sql" formatter="yql"}}
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
                        AsTuple('mode', coalesce($x[1],''))
                    )
                )
            }
        )
};



DEFINE SUBQUERY $active_users() AS
    DEFINE SUBQUERY $res($params) AS 
        SELECT 
            `date`,
            'active_users'||'_'||$format_name($params['mode']) as metric_id,
            'Active Users'||'_'||CAST($params['mode'] as Utf8)  as metric_name,
            'general' as metric_group,
            'general' as metric_subgroup,
            'BAs' as metric_unit, 
            0 as is_ytd,
            CAST($params['period'] AS Utf8) as period,
            count(DISTINCT billing_account_id) as metric_value
        FROM (
            SELECT 
                $round_period_datetime($params['period'], event_time) as `date`,
                billing_account_id
            FROM 
                `//home/cloud_analytics/cubes/acquisition_cube/cube` as acube
            LEFT JOIN
                `//home/cloud_analytics/export/billing/sku_tags/sku_tags` as sku
            ON acube.sku_id = sku.sku_id
            WHERE 
                event = 'day_use'
                and if($params['period'] = 'year',
                    $round_period_datetime($params['period'],event_time) = $round_current_time($params['period']),
                    $round_period_datetime($params['period'],event_time) < $round_current_time($params['period']))
                and acube.sku_lazy = 0
                and is_fraud = 0
                and real_consumption_vat > 0
                AND CASE
                    WHEN $params['mode'] = 'all' then sku.sku_id is not null
                    WHEN $params['mode'] = 'MDB' then sku.service_name = 'mdb'
                    WHEN $params['mode'] = 'Kubernetes' then (sku.service_name = 'mk8s' OR vm_origin = 'mk8s-worker') 
                    WHEN $params['mode'] = 'SpeechKit' then sku.subservice_name = 'cloud_ai'
                    WHEN $params['mode'] = 'IaaS' then sku.service_group = 'Infrastructure'
                    WHEN $params['mode'] = 'DataSphere' then sku.subservice_name = 'datasphere'
                    WHEN $params['mode'] = 'DataLens' then sku.service_name = 'datalens'
                    WHEN $params['mode'] = 'CloudFunctions' then sku.service_name = 'serverless'
                    WHEN $params['mode'] = 'YDB' then sku.service_name = 'ydb'
                    WHEN $params['mode'] = 'Monitoring' then sku.service_name = 'monitoring'
                    WHEN $params['mode'] = 'VAR' then is_var = 'var'
                    else sku.sku_id is not null
                END 
        )
        GROUP BY `date`
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
                        'MDB',
                        'Kubernetes',
                        'SpeechKit',
                        'IaaS',
                        'DataSphere',
                        'DataLens',
                        'CloudFunctions',
                        'YDB',
                        'Monitoring',
                        'VAR'
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $active_users;