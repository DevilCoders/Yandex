/*___DOC___
====Sustainable Revenue
Устойчивая выручка - выручка за неделю без ряда sku, потребление по которым начисляется в конце месяца
<{График
{{iframe frameborder="0" width="70%" height="300px" src="https://charts.yandex-team.ru/preview/editor/hv5twavvj2jqb?metric_name=Sustainable%20Revenue&6112cac5-7432-4748-a900-99da3f669075=Sustainable%20Revenue&metricName=Sustainable%20Revenue&_embedded=1"}}
}>
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/sustainable_revenue.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;

IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $revenue() AS 
    DEFINE SUBQUERY $revenue_ungrouped($param, $period) AS
        SELECT 
            $round_period_datetime($period,event_time) as `date`,
            real_consumption_vat
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube` as acube
        LEFT JOIN
            `//home/cloud_analytics/export/billing/sku_tags/sku_tags` as sku
        ON acube.sku_id = sku.sku_id
        WHERE 
            event = 'day_use'
            --and $format_date(DateTime::StartOfWeek($parse_datetime(event_time))) < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
            and if($period = 'year', $round_period_datetime($period,event_time) = $round_current_time($period),
                $round_period_datetime($period,event_time) < $round_current_time($period))
            AND CASE
                    WHEN $param = 'all' then sku.sku_id is not null
                    WHEN $param = 'MDB' then sku.service_name = 'mdb'
                    WHEN $param = 'Kubernetes' then sku.service_name = 'mk8s' --and sku.service_group = 'Data Storage and Analytics'
                    WHEN $param = 'SpeechKit' then sku.subservice_name = 'cloud_ai' and sku.subservice_name = 'speech'
                    WHEN $param = 'Translate' then sku.subservice_name = 'cloud_ai' and sku.subservice_name = 'mt'
                    WHEN $param = 'IaaS' then sku.service_group = 'Infrastructure'
                    WHEN $param = 'DataSphere' then sku.subservice_name = 'datasphere'
                    WHEN $param = 'DataLens' then sku.service_name = 'datalens'
                    WHEN $param = 'CloudFunctions' then sku.service_name = 'serverless'
                    WHEN $param = 'YDB' then sku.service_name = 'ydb'
                    WHEN $param = 'Monitoring' then sku.service_name = 'monitoring'
                    WHEN $param = 'DataPlatform' then sku.service_group = 'Data Storage and Analytics' and sku.service_name in ('datalens','mdb')
                    WHEN $param = 'ML_AI' then sku.service_group = 'ML and AI'
                    WHEN $param = 'Serverless' then sku.service_name in ('iot','storage','api_gateway','ymq','serverless','ydb')
                    WHEN $param = 'Marketplace' then sku.service_group = 'Marketplace'
                    else sku.sku_id is not null
                END 
    END DEFINE;
    DEFINE SUBQUERY $res($param,$period) AS 
        SELECT 
            `date`,
            'yc_revenue'||'_'||$param as metric_id,
            'YC Revenue'||'_'||$param as metric_name,
            'general' as metric_group,
            'money' as metric_subgroup,
            '₽' as metric_unit, 
            0 as is_ytd,
            $period as period,
            SUM(real_consumption_vat) as metric_value
        FROM $revenue_ungrouped($param,$period)
        GROUP BY `date`
    END DEFINE;


    DEFINE SUBQUERY $res_with_metric_growth1($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'all', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth2($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'MDB', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth3($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'Kubernetes', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth4($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'SpeechKit', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth5($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'IaaS', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth6($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'DataSphere', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth7($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'DataLens', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth8($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'CloudFunctions', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth9($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'YDB', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth10($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'Monitoring', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth11($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'DataPlatform', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth12($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'ML_AI', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth13($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'Serverless', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth14($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'Marketplace', $period, 'straight', 'standart', '')
        )
    END DEFINE;


    DEFINE SUBQUERY $res_with_metric_growth15($period) AS (
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
    $s11 = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth11
        );
    $s12 = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth12
        );
    $s13 = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth13
        );
    $s14 = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth14
        );   
    $s15 = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth15
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
    UNION ALL
    PROCESS $s11()
    UNION ALL
    PROCESS $s12()
    UNION ALL
    PROCESS $s13()
    UNION ALL
    PROCESS $s14()
    UNION ALL
    PROCESS $s15()

END DEFINE;

EXPORT $revenue;
