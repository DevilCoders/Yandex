/*___DOC___
====Enterprise Revenue
Выручка сегмента Enterprise по неделям
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/enterprise_revenue.sql" formatter="yql"}}
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
        $round_period_datetime($params['period'],event_time) as `date`,
        real_consumption_vat
    FROM 
        `//home/cloud_analytics/cubes/acquisition_cube/cube` as acube
    LEFT JOIN
        `//home/cloud_analytics/export/billing/sku_tags/sku_tags` as sku
    ON acube.sku_id = sku.sku_id
    WHERE 
        event = 'day_use'
        and CASE
            WHEN $params['segment'] = 'kz' THEN ba_person_type in ('kazakhstan_company','kazakhstan_individual')
            WHEN $params['segment'] = 'SMB' THEN crm_account_segment in ('Mass','Medium')
            WHEN $params['segment'] = 'Enterprise' THEN crm_account_segment in ('Enterprise')
            WHEN $params['segment'] = 'Mass' THEN crm_account_segment in ('Mass')
            WHEN $params['segment'] = 'Medium' THEN crm_account_segment in ('Medium')
            WHEN $params['segment'] = 'Public sector' THEN crm_account_segment in ('Public sector')
            WHEN $params['segment'] = 'partner_enterprise' THEN crm_account_segment in ('Enterprise') and is_var = 'var'
            WHEN $params['segment'] = 'partner_SMB' THEN crm_account_segment in ('Medium','Mass') and is_var = 'var' 
            WHEN $params['segment'] = 'partner_public_sector' THEN crm_account_segment in ('Public sector') and is_var = 'var' 
            WHEN $params['segment'] = 'partner_all' THEN is_var = 'var' 
            WHEN $params['segment'] = 'all' then sku.sku_id is not null
            WHEN $params['segment'] = 'MDB' then sku.service_name = 'mdb'
            WHEN $params['segment'] = 'Kubernetes' then (sku.service_name = 'mk8s' OR vm_origin = 'mk8s-worker')
            WHEN $params['segment'] = 'SpeechKit' then sku.service_name = 'cloud_ai' and sku.subservice_name = 'speech'
            WHEN $params['segment'] = 'Translate' then sku.service_name = 'cloud_ai' and sku.subservice_name = 'mt'
            WHEN $params['segment'] = 'IaaS' then sku.service_group = 'Infrastructure'
            WHEN $params['segment'] = 'DataSphere' then sku.subservice_name = 'datasphere'
            WHEN $params['segment'] = 'DataLens' then sku.service_name = 'datalens'
            WHEN $params['segment'] = 'CloudFunctions' then sku.service_name = 'serverless'
            WHEN $params['segment'] = 'YDB' then sku.service_name = 'ydb'
            WHEN $params['segment'] = 'Monitoring' then sku.service_name = 'monitoring'
            WHEN $params['segment'] = 'DataPlatform' then sku.service_group = 'Data Storage and Analytics' and sku.service_name in ('datalens','mdb')
            WHEN $params['segment'] = 'ML_AI' then sku.service_group = 'ML and AI'
            WHEN $params['segment'] = 'Serverless' then sku.service_name in ('iot','storage','api_gateway','ymq','serverless','ydb')
            WHEN $params['segment'] = 'Marketplace' then sku.service_group = 'Marketplace'
            WHEN $params['segment'] = 'Compute' then sku.service_name = 'compute'
            ELSE True
            END 
        and if($params['period'] = 'year', 
            $round_period_datetime($params['period'],event_time) = $round_current_time($params['period']),
            $round_period_datetime($params['period'],event_time) < $round_current_time($params['period']))
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
            sum(real_consumption_vat) as metric_value
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
                        'Public sector',
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


 