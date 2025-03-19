/*___DOC___
====New Customers
Количество биллинг-аккаунтов, начавших платное потребление на этой неделе.
Не учитываются аккаунты, потребляющие **только** sku с %%sku_lazy = 1%%.
Из выборки исключены фродовое (billing_account_is_suspended_by_antifraud на момент рассчитываемого потребления) клиенты.
Клиент считается фродовым по логике, описанной тут: https://st.yandex-team.ru/CLOUD-79491#61485d91ccf5186c3e578fa0

Как определеяется продукт/сегмент/сервис:
'Enterprise' => crm_segment in ('Enterprise')
'Public sector' => crm_segment in ('Public sector')
'SMB' => crm_segment in ('Mass', 'Medium') and billing_account_person_type = 'company'
'MDB' => sku_service_name = 'mdb'
'Kubernetes' => (sku_service_name = 'mk8s' OR billing_record_origin_service = 'mk8s')
'SpeechKit' => sku_service_name = 'cloud_ai' and sku_subservice_name = 'speech'
'IaaS' => sku_service_group = 'Infrastructure'
'Translate' => sku_service_name = 'cloud_ai' and sku_subservice_name = 'mt'

<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/new_customers.sql" formatter="yql"}}
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
            billing_record_msk_date
        FROM 
            `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption` as acube
        WHERE 
            billing_record_real_consumption_rub_vat > 0 
            and billing_account_is_suspended_by_antifraud = False
            and sku_lazy = 0
            AND CASE
                    WHEN $params['mode'] = 'Enterprise' THEN crm_segment in ('Enterprise')
                    WHEN $params['mode'] = 'Public sector' THEN crm_segment in ('Public sector')
                    WHEN $params['mode'] = 'SMB' THEN crm_segment in ('Mass', 'Medium') and billing_account_person_type = 'company'
                    WHEN $params['mode'] = 'MDB' then sku_service_name = 'mdb'
                    WHEN $params['mode'] = 'Kubernetes' then (sku_service_name = 'mk8s' OR billing_record_origin_service = 'mk8s')
                    WHEN $params['mode'] = 'SpeechKit' then sku_service_name = 'cloud_ai' and sku_subservice_name = 'speech'
                    WHEN $params['mode'] = 'IaaS' then sku_service_group = 'Infrastructure'
                    WHEN $params['mode'] = 'Translate' then sku_service_name = 'cloud_ai' and sku_subservice_name = 'mt'
                    ELSE True
                END 
    END DEFINE;

    DEFINE SUBQUERY $new_customers_group($params) AS
        SELECT  
            billing_account_id,
            min($round_period_date($params['period'],billing_record_msk_date)) as start_of_paid_period
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