/*___DOC___
====Conversion BA -> Trial
Конверсия в первое триальное потребление для недельной корогты созданных Billing Account.
Метрика показывает, у скольких Billing Account было триальное потребление в ту же календарную неделю, в которую эти Billing Account были созданы.
Из выборки исключены фродовые BA
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/marketing/conversion_ba_trial.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_from_second,$format_datetime,
 $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $conversion_ba_trial() AS


    

    DEFINE SUBQUERY $filters($params) AS 
        SELECT
            billing_account_id
        FROM 
            `//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags`
        WHERE 
            cast(`date` as Date) = CurrentUtcDate()
            and is_suspended_by_antifraud_current = False
            and person_type_current = 'company'
    END DEFINE;

    DEFINE SUBQUERY $first_trial($params) AS 
        SELECT
            $format_from_second(event_timestamp) as first_trial,
            event_entity_id as ba_trial
        FROM `//home/cloud-dwh/data/prod/cdm/dm_events`
        WHERE event_type = 'billing_account_first_trial_consumption'
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
            $round_period_datetime($params,$format_datetime(ba_created + INTERVAL('P7D'))) as `date`,
            if($params = 'week' and $format_date(first_trial) < $format_date(ba_created + INTERVAL('P7D')),
                1,
                if($params = 'month' and $format_date(first_trial) < $format_date(ba_created + INTERVAL('P7D')),
                    1,
                    0
                )
            ) as ba_conv,
            ba_created
        FROM $new_ba($params) as ba 
        LEFT JOIN $first_trial($params) as first_trial
        ON first_trial.ba_trial = ba.billing_account_id
        WHERE 
            ba_created <= coalesce(first_trial.first_trial,cast('2099-01-01' as Date))
            and ba_created + INTERVAL('P7D')  < CurrentUtcDate()
    END DEFINE;

    DEFINE SUBQUERY $res($params) AS
        SELECT 
            `date`,
            'conversion_ba_trial' as metric_id,
            'Conversion BA => Trial' as metric_name,
            'marketing' as metric_group,
            'marketing' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            cast($params as Utf8) as period,
            sum(ba_conv)/(count(distinct ba_created) + 0.0) * 100 as metric_value
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

EXPORT $conversion_ba_trial;