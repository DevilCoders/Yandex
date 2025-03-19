/*___DOC___
====Conversion Cloud -> BA
Конверсия в создание Billing Account для недельной корогты созданных Cloud_id.
Метрика показывает, сколько было создано Billing Account в ту же календарную неделю, в которую были созданы соответствующием им Cloud_id.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/marketing/conversion_cloud_ba.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $format_datetime,
$round_period_datetime, $round_period_date,  $round_current_time, $format_from_second;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $conversion_cloud_ba() AS
        
    DEFINE SUBQUERY $new_cloud($params) AS 
        SELECT
            $format_from_second(event_timestamp) as cloud_created,
            event_entity_id as cloud_id,
            billing_account_id
        FROM `//home/cloud-dwh/data/prod/cdm/dm_events`
        WHERE event_type = 'cloud_created'
    END DEFINE;

    DEFINE SUBQUERY $new_ba($params) AS 
        SELECT
            $format_from_second(event_timestamp) as ba_created,
            event_entity_id as billing_account_id
        FROM `//home/cloud-dwh/data/prod/cdm/dm_events`
        WHERE event_type = 'billing_account_created'
    END DEFINE;

    DEFINE SUBQUERY $res($params) AS
        SELECT
            $round_period_datetime($params,$format_datetime(cloud_created + INTERVAL('P7D'))) as `date`,
            if($params = 'week' and $format_date(ba_created) < $format_date(cloud_created + INTERVAL('P7D')),
                1,
                if($params = 'month' and $format_date(ba_created) < $format_date(cloud_created + INTERVAL('P31D')),
                    1,
                    0
                )
            ) as ba_conv,
            cloud_id
        FROM $new_cloud($params) as cloud
        LEFT JOIN $new_ba($params) as ba 
        ON cloud.billing_account_id = ba.billing_account_id
        WHERE 
            cloud_created <= coalesce(ba_created,cast('2099-01-01' as Date))
            and cloud_created + INTERVAL('P7D')  < CurrentUtcDate()
    END DEFINE;

    DEFINE SUBQUERY $res($params) AS
        SELECT 
            `date`,
            'conversion_cloud_ba' as metric_id,
            'Conversion Cloud => BA' as metric_name,
            'marketing' as metric_group,
            'marketing' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            cast($params as Utf8) as period,
            sum(ba_conv)/(count(distinct cloud_id) + 0.0) * 100 as metric_value
        FROM 
            $res($params)
        GROUP BY 
            `date`
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $params, 'straight', 'standart', '')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
            ['week','month'],
            $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $conversion_cloud_ba;