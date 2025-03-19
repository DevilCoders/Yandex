/*___DOC___
====New Customers Ent
Количество биллинг-аккаунтов, начавших платное потребление на этой неделе.
Не учитываются аккаунты, потребляющие **только** sku с %%sku_lazy = 1%%. Рассматриваются биллинг-аккаунты сегмента Enterprise.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/enterprise/new_customers_ent.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
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



DEFINE SUBQUERY $new_customers() AS
    DEFINE SUBQUERY $new_customers_e($params) AS
        SELECT
            billing_account_id,
            event_time
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube` as acube
        LEFT JOIN
            `//home/cloud_analytics/export/billing/sku_tags/sku_tags` as sku
        ON acube.sku_id = sku.sku_id
        WHERE 
            real_consumption_vat > 0 
            and acube.sku_lazy = 0
            AND CASE
                    WHEN $params['mode'] = 'Enterprise' THEN crm_account_segment in ('Enterprise')
                    WHEN $params['mode'] = 'Public sector' THEN crm_account_segment in ('Public sector')
                    WHEN $params['mode'] = 'SMB' THEN crm_account_segment in ('Mass', 'Medium') and ba_person_type = 'company'
                    WHEN $params['mode'] = 'MDB' then sku.service_name = 'mdb'
                    WHEN $params['mode'] = 'Kubernetes' then (sku.service_name = 'mk8s' OR vm_origin = 'mk8s-worker')
                    WHEN $params['mode'] = 'SpeechKit' then sku.service_name = 'cloud_ai' and sku.subservice_name = 'speech'
                    WHEN $params['mode'] = 'IaaS' then sku.service_group = 'Infrastructure'
                    WHEN $params['mode'] = 'Translate' then sku.service_name = 'cloud_ai' and sku.subservice_name = 'mt'
                    ELSE True
                END 
    END DEFINE;

    DEFINE SUBQUERY $new_customers_group($params) AS
        SELECT  
            billing_account_id,
            min($round_period_datetime($params['period'],event_time)) as start_of_paid_period
        FROM 
            $new_customers_e($params)
        GROUP BY 
            billing_account_id
    END DEFINE;

    DEFINE SUBQUERY $res($params) AS
        SELECT 
            start_of_paid_period as `date`,
            'new_customers_'||$format_name($params['mode']) as metric_id,
            'New Customers '||CAST($params['mode'] as Utf8) as metric_name,
            'general' as metric_group,
            'enterprise' as metric_subgroup,
            'BAs' as metric_unit,
            0 as is_ytd,
            CAST($params['period'] as Utf8) as period,
            count(DISTINCT billing_account_id) as metric_value
        FROM 
            $new_customers_group($params)
        GROUP BY start_of_paid_period
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
                        'all',
                        'Enterprise',
                        'Public sector',
                        'SMB',
                        'MDB',
                        'Kubernetes',
                        'SpeechKit',
                        'IaaS',
                        'Translate'
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()


END DEFINE;

EXPORT $new_customers;