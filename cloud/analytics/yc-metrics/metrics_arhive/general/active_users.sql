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

DEFINE SUBQUERY $active_users() AS
    DEFINE SUBQUERY $res($param,$period) AS 
        SELECT 
            `date`,
            'active_users'||'_'||$param  as metric_id,
            'Active Users'||'_'||$param  as metric_name,
            'general' as metric_group,
            'general' as metric_subgroup,
            'BAs' as metric_unit, 
            0 as is_ytd,
            $period as period,
            count(DISTINCT billing_account_id) as metric_value
        FROM (
            SELECT 
                $round_period_datetime($period,event_time) as `date`,
                billing_account_id
            FROM 
                `//home/cloud_analytics/cubes/acquisition_cube/cube` as acube
            LEFT JOIN
                `//home/cloud_analytics/export/billing/sku_tags/sku_tags` as sku
            ON acube.sku_id = sku.sku_id
            WHERE 
                event = 'day_use'
                and if($period = 'year',$round_period_datetime($period,event_time) = $round_current_time($period),
                    $round_period_datetime($period,event_time) < $round_current_time($period))
                and acube.sku_lazy = 0
                and is_fraud = 0
                and real_consumption_vat > 0
                AND CASE
                    WHEN $param = 'all' then sku.sku_id is not null
                    WHEN $param = 'MDB' then sku.service_name = 'mdb'
                    WHEN $param = 'Kubernetes' then sku.service_name = 'mk8s' and sku.service_group = 'Data Storage and Analytics'
                    WHEN $param = 'SpeechKit' then sku.subservice_name = 'cloud_ai'
                    WHEN $param = 'IaaS' then sku.service_group = 'Infrastructure'
                    WHEN $param = 'DataSphere' then sku.subservice_name = 'datasphere'
                    WHEN $param = 'DataLens' then sku.service_name = 'datalens'
                    WHEN $param = 'CloudFunctions' then sku.service_name = 'serverless'
                    WHEN $param = 'YDB' then sku.service_name = 'ydb'
                    WHEN $param = 'Monitoring' then sku.service_name = 'monitoring'
                    else sku.sku_id is not null
                END 
        )
        GROUP BY `date`
    END DEFINE;


    DEFINE SUBQUERY $res_with_metric_growth1($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'all', $period, 'straight', 'growth', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth2($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'MDB', $period, 'straight', 'growth', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth3($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'Kubernetes', $period, 'straight', 'growth', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth4($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'SpeechKit', $period, 'straight', 'growth', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth5($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'IaaS', $period, 'straight', 'growth', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth6($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'DataSphere', $period, 'straight', 'growth', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth7($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'DataLens', $period, 'straight', 'growth', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth8($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'CloudFunctions', $period, 'straight', 'growth', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth9($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'YDB', $period, 'straight', 'growth', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth10($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'Monitoring', $period, 'straight', 'growth', '')
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

    $s6 = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth6
        );

    $s7 = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth7
        );

    $s8 = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth8
        );

    $s9 = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth9
        );

    $s10 = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth10
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
    UNION ALL
    PROCESS $s6()
    UNION ALL
    PROCESS $s7()
    UNION ALL
    PROCESS $s8()
    UNION ALL
    PROCESS $s9()
    UNION ALL
    PROCESS $s10()


END DEFINE;

EXPORT $active_users;