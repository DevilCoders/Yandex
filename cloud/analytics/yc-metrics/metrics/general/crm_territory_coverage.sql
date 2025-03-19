/*___DOC___
====CRM Territory Coverage
Динамика отношения числа CRM-аккаунтов, у которых есть биллинг-аккаунт и есть платное потребление к числу всех CRM-аккаунтов, созданных на момент расчета метрики.
Расчитывается сегмента Enterprise.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/crm_territory_coverage.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time, $round_period_format;
IMPORT time_series SYMBOLS $metric_growth;


$format_name = ($name) -> { RETURN Unicode::ToLower(CAST(coalesce($name,'') as Utf8)) };

$script = @@#python
import itertools
def product(x):
    return itertools.product(*x, repeat=1)
@@;

$product = Python3::product(
    Callable<(List<List<String?>>)->List<List<String?>>>,
    $script
);

$make_param_dict = ($x) -> {
    RETURN 
        ListMap(
            $x,
            ($x) -> {
                RETURN ToDict(
                    AsList(
                        AsTuple('period', coalesce($x[0],'')),
                        AsTuple('mode', coalesce($x[1],''))
                    )
                )
            }
        )
};


DEFINE SUBQUERY $crm_territory_coverage() AS
$dates_range_ = (
    SELECT DISTINCT
        `date`,
        one 
    FROM (
        SELECT 
            $dates_range('2019-01-01', $format_date(CurrentUtcTimestamp())) as `date`,
            1 as one
    )
    FLATTEN BY `date`
);

$crm_accounts = (
    SELECT
        1 as one,
        acc_id,
        --$format_date(Datetime::FromSeconds(CAST(acc_date_entered AS Uint32))) as date_entered,
        $format_date($parse_datetime(acc_date_entered)) as date_entered,
        ba_id,
        business_segment
    FROM
        `//home/cloud_analytics/kulaga/acc_sales_ba_cube`
);


$ba_first_paid = (
    SELECT 
        DISTINCT billing_account_id,
        $format_date($parse_datetime(msk_event_dttm)) as ba_first_paid_date
    FROM 
        `//home/cloud-dwh/data/prod/cdm/dm_events`
    WHERE 
        event_type = 'billing_account_first_paid_consumption'
);

DEFINE SUBQUERY $accounts_raw($params) AS
    SELECT
        `date`,
        $round_period_date($params['period'],`date`) as date_week,
        acc_id,
        date_entered,
        business_segment,
        ba_id,
        if(ba_first_paid_date < `date`, ba_first_paid_date, null) as ba_first_paid_date
    FROM 
        $dates_range_ as dates_range
    LEFT JOIN
        $crm_accounts as crm_accounts 
    ON dates_range.one = crm_accounts.one 
    LEFT JOIN 
        $ba_first_paid as ba_first_paid
    ON crm_accounts.ba_id = ba_first_paid.billing_account_id
END DEFINE;

DEFINE SUBQUERY $accounts_with_ba($params) AS
    SELECT  
        date_week,
        count(DISTINCT acc_id) + 0.0 as accounts_with_ba
    FROM 
        $accounts_raw($params)
    WHERE 
        `date` >= date_entered 
        AND business_segment = $params['mode']
        AND ba_first_paid_date is not null
    GROUP BY 
        date_week
END DEFINE;

DEFINE SUBQUERY $accounts_all($params) AS
    SELECT  
        date_week,
        count(DISTINCT acc_id) + 0.0 as accounts_all
    FROM 
        $accounts_raw($params)
    WHERE 
        `date` >= date_entered 
        AND business_segment = $params['mode']
    GROUP BY 
        date_week
END DEFINE;

DEFINE SUBQUERY $res($params) AS
SELECT 
    accounts_all.date_week as `date`,
    'crm_territory_coverage_'||$format_name($params['mode']) as metric_id,
    'CRM Territory coverage (' || cast($params['mode'] as Utf8) || ')' as metric_name,
    'general' as metric_group,
    'general' as metric_subgroup,
    '%' as metric_unit, 
    0 as is_ytd,
    cast($params['period'] as Utf8) as period,
    coalesce(accounts_with_ba, 0) / accounts_all * 100.0 as metric_value
FROM 
    $accounts_all($params) as accounts_all
LEFT JOIN 
    $accounts_with_ba($params) as accounts_with_ba 
ON accounts_all.date_week = accounts_with_ba.date_week
END DEFINE;


    DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $params, 'straight', 'standart', '')
        )
    END DEFINE;

    $s = SubqueryUnionAllFor(
        $make_param_dict(
            $product(
                AsList(
                    AsList(
                        'day', 
                        'week',
                        'month', 
                        'quarter',
                        'year'
                    ), 
                    AsList(
                        'Mass',
                        'Medium',
                        'Enterprise',
                        'Public sector'
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $crm_territory_coverage;
