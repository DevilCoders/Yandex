/*___DOC___
====New Customers
Количество биллинг-аккаунтов c ba_person_type = 'company', начавших платное потребление на этой неделе.
Не учитываются аккаунты, потребляющие **только** sku с %%sku_lazy = 1%%. Рассматриваются биллинг-аккаунты с сегментом Mass, Medium.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/SMB/new_customers.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $new_customers() AS
    DEFINE SUBQUERY $new_customers($period) AS
        SELECT
            billing_account_id,
            min($round_period_datetime($period,event_time)) as start_of_paid_period
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 
            real_consumption_vat > 0 
            and sku_lazy = 0
            and crm_account_segment in ('Mass', 'Medium')
            and ba_person_type = 'company'
        GROUP BY 
            billing_account_id
    END DEFINE;

    DEFINE SUBQUERY $res($param,$period) AS
        SELECT 
            start_of_paid_period as `date`,
            'new_customers' as metric_id,
            'New Customers' as metric_name,
            'SMB' as metric_group,
            'SMB' as metric_subgroup,
            'BAs' as metric_unit, 
            0 as is_ytd,
            $period as period,
            count(billing_account_id) as metric_value
        FROM 
            $new_customers($period)
        GROUP BY start_of_paid_period
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

EXPORT $new_customers;