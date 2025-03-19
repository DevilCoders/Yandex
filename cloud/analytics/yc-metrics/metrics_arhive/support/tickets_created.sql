/*___DOC___
==== Tickets Created All
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/tickets_created_all.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;
IMPORT lib_tickets_created SYMBOLS $lib_tickets_created;

DEFINE SUBQUERY $tickets_created() AS 

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
            tickets_count as metric_value
        FROM $lib_tickets_created($params,$period)
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth1($period) AS (

        SELECT * FROM 
        $metric_growth(
            $res, 
            AsDict(
                AsTuple(
                    'metric_id',
                    AsList('support_tickets_count_all')
                ),
                AsTuple(
                    'metric_name',
                    AsList('Support - Number of Tickets (All)')
                ),
                AsTuple(
                    'filter_field',
                    AsList('no_filter')
                ),
                AsTuple(
                    'filter_values',
                    AsList('no_filter')
                )
            ),$period,
            'inverse','standart',''
        )
    )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth2($period) AS (

        SELECT * FROM 
        $metric_growth(
            $res, 
            AsDict(
                AsTuple(
                    'metric_id',
                    AsList('support_tickets_count_ba_paid_status_paid')
                ),
                AsTuple(
                    'metric_name',
                    AsList('Support - Number of Tickets (BA Paid Status: Paid)')
                ),
                AsTuple(
                    'filter_field',
                    AsList('ba_paid_status')
                ),
                AsTuple(
                    'filter_values',
                    AsList('paid')
                )
            ),$period,
            'inverse','standart',''
        )
    )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth3($period) AS (
        SELECT * FROM 
        $metric_growth(
            $res, 
            AsDict(
                AsTuple(
                    'metric_id',
                    AsList('support_tickets_count_ba_paid_status_trial')
                ),
                AsTuple(
                    'metric_name',
                    AsList('Support - Number of Tickets (BA Paid Status: Trial)')
                ),
                AsTuple(
                    'filter_field',
                    AsList('ba_paid_status')
                ),
                AsTuple(
                    'filter_values',
                    AsList('trial')
                )
            ),$period,
            'inverse','standart',''
        )
    )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth4($period) AS (

        SELECT * FROM 
        $metric_growth(
            $res, 
            AsDict(
                AsTuple(
                    'metric_id',
                    AsList('support_tickets_count_segment_mass')
                ),
                AsTuple(
                    'metric_name',
                    AsList('Support - Number of Tickets (Segment: Mass)')
                ),
                AsTuple(
                    'filter_field',
                    AsList('crm_account_segment')
                ),
                AsTuple(
                    'filter_values',
                    AsList('Mass')
                )
            ),$period,
            'inverse','standart',''
        )
    ) 
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth5($period) AS (

        SELECT * FROM 
        $metric_growth(
            $res, 
            AsDict(
                AsTuple(
                    'metric_id',
                    AsList('support_tickets_count_segment_medium')
                ),
                AsTuple(
                    'metric_name',
                    AsList('Support - Number of Tickets (Segment: Medium)')
                ),
                AsTuple(
                    'filter_field',
                    AsList('crm_account_segment')
                ),
                AsTuple(
                    'filter_values',
                    AsList('Medium')
                )
            ),$period,
            'inverse','standart',''
        )
    )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth6($period) AS (

        SELECT * FROM 
        $metric_growth(
            $res, 
            AsDict(
                AsTuple(
                    'metric_id',
                    AsList('support_tickets_count_segment_enterprise')
                ),
                AsTuple(
                    'metric_name',
                    AsList('Support - Number of Tickets (Segment: Enterprise)')
                ),
                AsTuple(
                    'filter_field',
                    AsList('crm_account_segment')
                ),
                AsTuple(
                    'filter_values',
                    AsList('Enterprise')
                )
            ),$period,
            'inverse','standart',''
        )
    ) 
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth7($period) AS (

        SELECT * FROM 
        $metric_growth(
            $res, 
            AsDict(
                AsTuple(
                    'metric_id',
                    AsList('support_tickets_count_support_type_free')
                ),
                AsTuple(
                    'metric_name',
                    AsList('Support - Number of Tickets (Support Type: Free)')
                ),
                AsTuple(
                    'filter_field',
                    AsList('ba_support_type')
                ),
                AsTuple(
                    'filter_values',
                    AsList('free')
                )
            ),$period,
            'inverse','standart',''
        )
    )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth8($period) AS (
        SELECT * FROM 
        $metric_growth(
            $res, 
            AsDict(
                AsTuple(
                    'metric_id',
                    AsList('support_tickets_count_support_type_standard')
                ),
                AsTuple(
                    'metric_name',
                    AsList('Support - Number of Tickets (Support Type: Standard)')
                ),
                AsTuple(
                    'filter_field',
                    AsList('ba_support_type')
                ),
                AsTuple(
                    'filter_values',
                    AsList('standard')
                )
            ),$period,
            'inverse','standart',''
        )
    ) 
    END DEFINE;


    DEFINE SUBQUERY $res_with_metric_growth9($period) AS (

        SELECT * FROM 
        $metric_growth(
            $res, 
            AsDict(
                AsTuple(
                    'metric_id',
                    AsList('support_tickets_count_support_type_business')
                ),
                AsTuple(
                    'metric_name',
                    AsList('Support - Number of Tickets (Support Type: Business)')
                ),
                AsTuple(
                    'filter_field',
                    AsList('ba_support_type')
                ),
                AsTuple(
                    'filter_values',
                    AsList('business')
                )
            ),$period,
            'inverse','standart',''
        )
    )
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth10($period) AS (

        SELECT * FROM 
        $metric_growth(
            $res, 
            AsDict(
                AsTuple(
                    'metric_id',
                    AsList('support_tickets_count_support_type_premium')
                ),
                AsTuple(
                    'metric_name',
                    AsList('Support - Number of Tickets (Support Type: Premium)')
                ),
                AsTuple(
                    'filter_field',
                    AsList('ba_support_type')
                ),
                AsTuple(
                    'filter_values',
                    AsList('premium')
                )
            ),$period,
            'inverse','standart',''
        )
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

EXPORT $tickets_created;