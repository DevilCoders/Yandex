/*___DOC___
==== Tickets Created Per BA
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/tickets_created_all.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;
IMPORT lib_tickets_created SYMBOLS $lib_tickets_created;
IMPORT lib_ba_count SYMBOLS $lib_ba_count;

DEFINE SUBQUERY $ba_count() AS 
    DEFINE SUBQUERY $res($params,$period) AS 
        SELECT 
            `date`,
            $params['metric_id'][0] as metric_id,
            $params['metric_name'][0] as metric_name,
            'support' as metric_group,
            'support' as metric_subgroup,
            '' as metric_unit, 
            0 as is_ytd,
            $period as period,
            ba_count as metric_value
        FROM $lib_ba_count($params)
    END DEFINE; 

    SELECT * FROM 
    $metric_growth(
        $res, 
        AsDict(
            AsTuple(
                'metric_id',
                AsList('ba_count_all')
            ),
            AsTuple(
                'metric_name',
                AsList('BA Count (All)')
            ),
            AsTuple(
                'filter_field',
                AsList('no_filter')
            ),
            AsTuple(
                'filter_values',
                AsList('no_filter')
            )
        ),
        'inverse','standart', ''
    )

    UNION ALL 

    SELECT * FROM 
    $metric_growth(
        $res, 
        AsDict(
            AsTuple(
                'metric_id',
                AsList('ba_count_ba_paid_status_paid')
            ),
            AsTuple(
                'metric_name',
                AsList('BA Count (BA Paid Status: Paid)')
            ),
            AsTuple(
                'filter_field',
                AsList('ba_paid_status')
            ),
            AsTuple(
                'filter_values',
                AsList('paid')
            )
        ),
        'inverse','standart', ''
    )

    UNION ALL 

    SELECT * FROM 
    $metric_growth(
        $res, 
        AsDict(
            AsTuple(
                'metric_id',
                AsList('ba_count_ba_paid_status_trial')
            ),
            AsTuple(
                'metric_name',
                AsList('BA Count (BA Paid Status: Trial)')
            ),
            AsTuple(
                'filter_field',
                AsList('ba_paid_status')
            ),
            AsTuple(
                'filter_values',
                AsList('trial')
            )
        ),
        'inverse','standart', ''
    )
    UNION ALL 

    SELECT * FROM 
    $metric_growth(
        $res, 
        AsDict(
            AsTuple(
                'metric_id',
                AsList('ba_count_segment_mass')
            ),
            AsTuple(
                'metric_name',
                AsList('BA Count (Segment: Mass)')
            ),
            AsTuple(
                'filter_field',
                AsList('crm_account_segment')
            ),
            AsTuple(
                'filter_values',
                AsList('Mass')
            )
        ),
        'inverse','standart', ''
    )

    UNION ALL 

    SELECT * FROM 
    $metric_growth(
        $res, 
        AsDict(
            AsTuple(
                'metric_id',
                AsList('ba_count_segment_medium')
            ),
            AsTuple(
                'metric_name',
                AsList('BA Count (Segment: Medium)')
            ),
            AsTuple(
                'filter_field',
                AsList('crm_account_segment')
            ),
            AsTuple(
                'filter_values',
                AsList('Medium')
            )
        ),
        'inverse','standart', ''
    )

    UNION ALL 

    SELECT * FROM 
    $metric_growth(
        $res, 
        AsDict(
            AsTuple(
                'metric_id',
                AsList('ba_count_segment_enterprise')
            ),
            AsTuple(
                'metric_name',
                AsList('BA Count (Segment: Enterprise)')
            ),
            AsTuple(
                'filter_field',
                AsList('crm_account_segment')
            ),
            AsTuple(
                'filter_values',
                AsList('Enterprise')
            )
        ),
        'inverse','standart', ''
    )

    UNION ALL 

    SELECT * FROM 
    $metric_growth(
        $res, 
        AsDict(
            AsTuple(
                'metric_id',
                AsList('ba_count_support_type_free')
            ),
            AsTuple(
                'metric_name',
                AsList('BA Count (Support Type: Free)')
            ),
            AsTuple(
                'filter_field',
                AsList('ba_support_type')
            ),
            AsTuple(
                'filter_values',
                AsList('free')
            )
        ),
        'inverse','standart', ''
    )

    UNION ALL 

    SELECT * FROM 
    $metric_growth(
        $res, 
        AsDict(
            AsTuple(
                'metric_id',
                AsList('ba_count_support_type_standard')
            ),
            AsTuple(
                'metric_name',
                AsList('BA Count (Support Type: Standard)')
            ),
            AsTuple(
                'filter_field',
                AsList('ba_support_type')
            ),
            AsTuple(
                'filter_values',
                AsList('standard')
            )
        ),
        'inverse','standart', ''
    )

    UNION ALL 

    SELECT * FROM 
    $metric_growth(
        $res, 
        AsDict(
            AsTuple(
                'metric_id',
                AsList('ba_count_support_type_business')
            ),
            AsTuple(
                'metric_name',
                AsList('BA Count (Support Type: Business)')
            ),
            AsTuple(
                'filter_field',
                AsList('ba_support_type')
            ),
            AsTuple(
                'filter_values',
                AsList('business')
            )
        ),
        'inverse','standart', ''
    )

    UNION ALL 

    SELECT * FROM 
    $metric_growth(
        $res, 
        AsDict(
            AsTuple(
                'metric_id',
                AsList('ba_count_support_type_premium')
            ),
            AsTuple(
                'metric_name',
                AsList('BA Count (Support Type: Premium)')
            ),
            AsTuple(
                'filter_field',
                AsList('ba_support_type')
            ),
            AsTuple(
                'filter_values',
                AsList('premium')
            )
        ),
        'inverse','standart', ''
    )
    

END DEFINE;

EXPORT $ba_count;

