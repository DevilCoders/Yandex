/*___DOC___
====Active Users 
Число активных пользователей за выбранный период.
Активным пользователем считается Billing Account, по которому было платное осознанное (sku_lazy = 0) потребление в течение этого периода.
Из выборки исключены фродовое (billing_account_is_suspended_by_antifraud на момент рассчитываемого потребления) клиенты.
Клиент считается фродовым по логике, описанной тут: https://st.yandex-team.ru/CLOUD-79491#61485d91ccf5186c3e578fa0

Как определеяется продукт/сегмент/сервис:
'all' => sku_id is not null
'MDB' => sku_service_name = 'mdb'
'Kubernetes' => (sku_service_name = 'mk8s' OR billing_record_origin_service = 'mk8s') 
'SpeechKit' => sku_subservice_name = 'cloud_ai'
'IaaS' => sku_service_group = 'Infrastructure'
'DataSphere' => sku_subservice_name = 'datasphere'
'DataLens' => sku_service_name = 'datalens'
'CloudFunctions' => sku_service_name = 'serverless'
'YDB' => sku_service_name = 'ydb'
'Monitoring' => sku_service_name = 'monitoring'
'VAR' => (billing_account_is_var = True OR billing_account_is_subaccount = True)


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
                $round_period_date($params['period'], billing_record_msk_date) as `date`,
                billing_account_id
            FROM 
                `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption` as acube
            WHERE 
                if($params['period'] = 'year',
                    $round_period_date($params['period'],billing_record_msk_date) = $round_current_time($params['period']),
                    $round_period_date($params['period'],billing_record_msk_date) < $round_current_time($params['period']))
                and acube.sku_lazy = 0
                and billing_account_is_suspended_by_antifraud = False
                and billing_record_real_consumption_rub_vat > 0
                AND CASE
                    WHEN $params['mode'] = 'all' then sku_id is not null
                    WHEN $params['mode'] = 'MDB' then sku_service_name = 'mdb'
                    WHEN $params['mode'] = 'Kubernetes' then (sku_service_name = 'mk8s' OR billing_record_origin_service = 'mk8s') 
                    WHEN $params['mode'] = 'SpeechKit' then sku_subservice_name = 'cloud_ai'
                    WHEN $params['mode'] = 'IaaS' then sku_service_group = 'Infrastructure'
                    WHEN $params['mode'] = 'DataSphere' then sku_subservice_name = 'datasphere'
                    WHEN $params['mode'] = 'DataLens' then sku_service_name = 'datalens'
                    WHEN $params['mode'] = 'CloudFunctions' then sku_service_name = 'serverless'
                    WHEN $params['mode'] = 'YDB' then sku_service_name = 'ydb'
                    WHEN $params['mode'] = 'Monitoring' then sku_service_name = 'monitoring'
                    WHEN $params['mode'] = 'VAR' then (billing_account_is_var = True OR billing_account_is_subaccount = True)
                    else sku_id is not null
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