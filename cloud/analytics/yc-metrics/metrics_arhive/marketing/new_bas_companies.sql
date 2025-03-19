/*___DOC___
====New BAs (Companies)
Количество биллинг-аккаунтов-юрлиц (%%ba_person_type='company'%%), зарегистрировавшихся за неделю за исключением фрода (%%is_fraud = 0%%)
<{График
{{iframe frameborder="0" width="70%" height="300px" src="https://charts.yandex-team.ru/preview/editor/hv5twavvj2jqb?metric_name=New%20BAs%20(Companies)&6112cac5-7432-4748-a900-99da3f669075=New%20BAs%20(Companies)&metricName=New%20BAs%20(Companies)&_embedded=1"}}
}>
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/marketing/new_bas_companies.sql" formatter="yql"}}
}>
___DOC___*/


use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $new_bas_companies() AS
    DEFINE SUBQUERY $res($param,$period) AS
        SELECT 
            `date`,
            'new_ba_companies' as metric_id,
            'New BAs (Companies)' as metric_name,
            'marketing' as metric_group,
            'marketing' as metric_subgroup,
            '' as metric_unit, 
            0 as is_ytd,
            $period as period,
            count(billing_account_id) as metric_value
        FROM (
        SELECT 
            --$format_date(DateTime::StartOfWeek($parse_datetime(event_time))) as `date`,
            $round_period_datetime($period,event_time) as `date`,
            billing_account_id
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 1=1
            and event = 'ba_created'
            and is_fraud = 0
            --and $format_date(DateTime::StartOfWeek($parse_datetime(event_time))) < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
            and if($period = 'year', $round_period_datetime($period,event_time) = $round_current_time($period),
                $round_period_datetime($period,event_time) < $round_current_time($period))
            and ba_person_type = 'company'
        )
        GROUP BY `date`
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, '', $period, 'straight', 'standart', '')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
            ['day','week','month','quarter','year'],
            $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $new_bas_companies;