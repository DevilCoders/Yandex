/*___DOC___
====Conversion Cloud -> BA  (Company)
Конверсия в создание Billing Account для недельной корогты созданных Cloud_id для Юрлиц.
Метрика показывает, сколько было создано Billing Account в ту же календарную неделю, в которую были созданы соответствующием им Cloud_id.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/marketing/conversion_cloud_ba.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $conversion_cloud_ba() AS
    DEFINE SUBQUERY $new_ba($period) AS
        SELECT
            $round_period_datetime($period,event_time) as `date`,
            if($format_date($parse_datetime(event_time)) <= $format_date($parse_datetime(first_ba_created_datetime))
                AND $format_date($parse_datetime(first_ba_created_datetime)) < $format_date(DateTime::MakeDatetime(DateTime::StartOfWeek($parse_datetime(event_time))) + INTERVAL('P7D')), 1, 0) as ba_created,
            cloud_id
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 
            event = 'cloud_created'
            --and ba_person_type = 'company'
            and is_fraud = 0
    END DEFINE;

    DEFINE SUBQUERY $res($param,$period) AS
        SELECT 
            `date`,
            'conversion_cloud_ba' as metric_id,
            'Conversion Cloud => BA' as metric_name,
            'marketing' as metric_group,
            'marketing' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
                $period as period,
            sum(ba_created)/(count(distinct cloud_id) + 0.0) * 100 as metric_value
        FROM 
            $new_ba($period)
        GROUP BY 
            `date`
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, '', $period, 'straight', 'growth', '')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
            ['week'],
            $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $conversion_cloud_ba;