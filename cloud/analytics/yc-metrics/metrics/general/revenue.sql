/*___DOC___
====Revenue

Выручка в разбивке на продукты/сегменты/сервисы.
Учитывается вся выручка (все sku, весь фрод).
Выручка за вычетом НДС (billing_record_real_consumption_rub_vat).
Значения сегментов и других флагов учитываются на момент  потребления.

Как определеяется продукт/сегмент/сервис:
'kz' => billing_account_person_type in ('kazakhstan_company','kazakhstan_individual') or billing_account_country_code = 'KZ'
'SMB' => crm_segment in ('Mass','Medium')
'Enterprise' => crm_segment in ('Enterprise')
'Mass' => crm_segment in ('Mass')
'Medium' => crm_segment in ('Medium')
'Public_sector' => crm_segment in ('Public sector')
'partner_enterprise' => crm_segment in ('Enterprise') and (billing_account_is_var = True or billing_account_is_subaccount = True)
'partner_SMB' => crm_segment in ('Medium','Mass') and (billing_account_is_var = True or billing_account_is_subaccount = True)
'partner_public_sector' => crm_segment in ('Public sector') and (billing_account_is_var = True or billing_account_is_subaccount = True)
'partner_all' => (billing_account_is_var = True or billing_account_is_subaccount = True)
'all' => sku_id is not null
'MDB' => sku_service_name = 'mdb'
'Kubernetes' => (sku_service_name = 'mk8s' OR billing_record_origin_service = 'mk8s')
'SpeechKit' => sku_service_name = 'cloud_ai' and sku_subservice_name = 'speech'
'Translate' => sku_service_name = 'cloud_ai' and sku_subservice_name = 'mt'
'IaaS' => sku_service_group = 'Infrastructure'
'DataSphere' => sku_subservice_name = 'datasphere'
'DataLens' => sku_service_name = 'datalens'
'CloudFunctions' => sku_service_name = 'serverless'
'YDB' => sku_service_name = 'ydb'
'Monitoring' => sku_service_name = 'monitoring'
'DataPlatform' => sku_service_group = 'Data Storage and Analytics' and sku_service_name in ('datalens','mdb')
'ML_AI' => sku_service_group = 'ML and AI'
'Serverless' => sku_service_name in ('iot','api_gateway','ymq','serverless','ydb')
'Marketplace' => sku_service_group = 'Marketplace'
'Compute' => sku_service_name = 'compute'
 
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/revenue.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

PRAGMA AnsiInForEmptyOrNullableItemsCollections;

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
                        AsTuple('segment', coalesce($x[1],''))
                    )
                )
            }
        )
};



DEFINE SUBQUERY $revenue() AS
    DEFINE SUBQUERY $revenue_ungrouped($params) 
    AS 
    SELECT 
        $round_period_date($params['period'],billing_record_msk_date) as `date`,
        -- billing_record_real_consumption_rub_vat
        billing_record_total_redistribution_rub_vat
    FROM 
        `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption` as acube
    WHERE 
        CASE
            WHEN $params['segment'] = 'kz' THEN (billing_account_person_type in ('kazakhstan_company','kazakhstan_individual') or billing_account_country_code = 'KZ')
            WHEN $params['segment'] = 'SMB' THEN crm_segment in ('Mass','Medium')
            WHEN $params['segment'] = 'Enterprise' THEN crm_segment in ('Enterprise')
            WHEN $params['segment'] = 'Mass' THEN crm_segment in ('Mass')
            WHEN $params['segment'] = 'Medium' THEN crm_segment in ('Medium')
            WHEN $params['segment'] = 'Public_sector' THEN crm_segment in ('Public sector')
            WHEN $params['segment'] = 'partner_enterprise' THEN crm_segment in ('Enterprise') and (billing_account_is_var = True or billing_account_is_subaccount = True)
            WHEN $params['segment'] = 'partner_SMB' THEN crm_segment in ('Medium','Mass') and (billing_account_is_var = True or billing_account_is_subaccount = True)
            WHEN $params['segment'] = 'partner_public_sector' THEN crm_segment in ('Public sector') and (billing_account_is_var = True or billing_account_is_subaccount = True)
            WHEN $params['segment'] = 'partner_all' THEN (billing_account_is_var = True or billing_account_is_subaccount = True)
            WHEN $params['segment'] = 'all' then sku_id is not null
            WHEN $params['segment'] = 'MDB' then sku_service_name = 'mdb'
            WHEN $params['segment'] = 'Kubernetes' then (sku_service_name = 'mk8s' OR billing_record_origin_service = 'mk8s')
            WHEN $params['segment'] = 'SpeechKit' then sku_service_name = 'cloud_ai' and sku_subservice_name = 'speech'
            WHEN $params['segment'] = 'Translate' then sku_service_name = 'cloud_ai' and sku_subservice_name = 'mt'
            WHEN $params['segment'] = 'IaaS' then sku_service_group = 'Infrastructure'
            WHEN $params['segment'] = 'DataSphere' then sku_subservice_name = 'datasphere'
            WHEN $params['segment'] = 'DataLens' then sku_service_name = 'datalens'
            WHEN $params['segment'] = 'CloudFunctions' then sku_service_name = 'serverless'
            WHEN $params['segment'] = 'YDB' then sku_service_name = 'ydb'
            WHEN $params['segment'] = 'Monitoring' then sku_service_name = 'monitoring'
            WHEN $params['segment'] = 'DataPlatform' then sku_service_group = 'Data Storage and Analytics' and sku_service_name in ('datalens','mdb')
            WHEN $params['segment'] = 'ML_AI' then sku_service_group = 'ML and AI'
            WHEN $params['segment'] = 'Serverless' then sku_service_name in ('iot','api_gateway','ymq','serverless','ydb')
            WHEN $params['segment'] = 'Marketplace' then sku_service_group = 'Marketplace'
            WHEN $params['segment'] = 'Compute' then sku_service_name = 'compute'
            ELSE True
            END 
        and if($params['period'] = 'year', 
            $round_period_date($params['period'],billing_record_msk_date) = $round_current_time($params['period']),
            $round_period_date($params['period'],billing_record_msk_date) < $round_current_time($params['period']))
    END DEFINE;

    DEFINE SUBQUERY $res($params) AS 
        SELECT 
            `date`,
            'revenue_'||$format_name($params['segment']) as metric_id,
            'Revenue '||CAST($params['segment'] as Utf8) as metric_name,
            'general' as metric_group,
            'money' as metric_subgroup,
            '₽' as metric_unit, 
            0 as is_ytd,
            CAST($params['period'] AS Utf8) as period,
            -- sum(billing_record_real_consumption_rub_vat) as metric_value
            sum(billing_record_total_redistribution_rub_vat) as metric_value
        FROM $revenue_ungrouped($params)
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
                        'day', 
                        'week',
                        'month', 
                        'quarter',
                        'year'
                    ), 
                    AsList(
                        'Enterprise',
                        'Medium',
                        'Mass',
                        'SMB',
                        'Public_sector',
                        'partner_enterprise',
                        'partner_SMB',
                        'partner_public_sector',
                        'partner_all',
                        'kz',
                        'all',               
                        'MDB',           
                        'Kubernetes',    
                        'SpeechKit',   
                        'Translate',     
                        'IaaS',          
                        'DataSphere', 
                        'DataLens',      
                        'CloudFunctions',
                        'YDB',           
                        'Monitoring',    
                        'DataPlatform',  
                        'ML_AI',         
                        'Serverless',    
                        'Marketplace',
                        'Compute'  
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $revenue;


 
