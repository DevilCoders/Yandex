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

DEFINE SUBQUERY $new_customers_product() AS
    DEFINE SUBQUERY $new_customers_e($param,$period) AS
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
                    WHEN $param = 'MDB' then sku.service_name = 'mdb'
                    WHEN $param = 'Kubernetes' then sku.service_name = 'mk8s' and sku.service_group = 'Data Storage and Analytics'
                    WHEN $param = 'SpeechKit' then sku.subservice_name = 'cloud_ai'
                    WHEN $param = 'IaaS' then sku.service_group = 'Infrastructure'
                    WHEN $param = 'Translate' then sku.subservice_name = 'cloud_ai' and sku.subservice_name = 'mt'
                    else sku.sku_id is not null
                END 
    END DEFINE;

    DEFINE SUBQUERY $new_customers_group($param,$period) AS
        SELECT  
            billing_account_id,
            min($round_period_datetime($period,event_time)) as start_of_paid_period
        FROM 
            $new_customers_e($param,$period)
        GROUP BY 
            billing_account_id
    END DEFINE;

    DEFINE SUBQUERY $res($param,$period) AS
        SELECT 
            start_of_paid_period as `date`,
            'new_customers_'||($param) as metric_id,
            'New Customers '||($param) as metric_name,
            'general' as metric_group,
            'general' as metric_subgroup,
            'BAs' as metric_unit,
            0 as is_ytd,
            $period as period,
            count(billing_account_id) as metric_value
        FROM 
            $new_customers_group($param,$period)
        GROUP BY start_of_paid_period
    END DEFINE;

DEFINE SUBQUERY $res_with_metric_growth1($period) AS (
    SELECT 
        *
    FROM 
        $metric_growth($res, 'MDB', $period, 'straight', 'standart', '')
    )
END DEFINE;

DEFINE SUBQUERY $res_with_metric_growth2($period) AS (
    SELECT 
        *
    FROM 
        $metric_growth($res, 'Kubernetes', $period, 'straight', 'standart', '')
    )
END DEFINE;

DEFINE SUBQUERY $res_with_metric_growth3($period) AS (
    SELECT 
        *
    FROM 
        $metric_growth($res, 'SpeechKit', $period, 'straight', 'standart', '')
    )
END DEFINE;

DEFINE SUBQUERY $res_with_metric_growth4($period) AS (
    SELECT 
        *
    FROM 
        $metric_growth($res, 'IaaS', $period, 'straight', 'standart', '')
    )
END DEFINE;

DEFINE SUBQUERY $res_with_metric_growth5($period) AS (
    SELECT 
        *
    FROM 
        $metric_growth($res, 'Translate', $period, 'straight', 'standart', '')
    )
END DEFINE;

$s1 = SubqueryUnionAllFor(
        ['day','week','month','quarter','year'],
        $res_with_metric_growth1
    );
$s2 = SubqueryUnionAllFor(
        ['day','week','month','quarter','year'],
        $res_with_metric_growth2
    );
$s3 = SubqueryUnionAllFor(
        ['day','week','month','quarter','year'],
        $res_with_metric_growth3
    );
$s4 = SubqueryUnionAllFor(
        ['day','week','month','quarter','year'],
        $res_with_metric_growth4
    );
$s5 = SubqueryUnionAllFor(
        ['day','week','month','quarter','year'],
        $res_with_metric_growth5
    );
PROCESS $s1()
UNION ALL
PROCESS $s2()
UNION ALL
PROCESS $s3()
UNION ALL
PROCESS $s4()
UNION ALL
PROCESS $s5()

END DEFINE;

EXPORT $new_customers_product;