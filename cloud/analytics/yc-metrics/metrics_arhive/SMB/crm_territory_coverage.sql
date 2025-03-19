/*___DOC___
====CRM Territory Coverage
Динамика отношения числа CRM-аккаунтов, у которых есть биллинг-аккаунт и есть платное потребление к числу всех CRM-аккаунтов, созданных на момент расчета метрики.
Расчитывается сегмента Enterprise.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/SMB/crm_territory_coverage.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time, $round_period_format;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $crm_territory_coverage() AS
$dates_range_ = (
    SELECT DISTINCT
        `date`,
        -- $format_date(DateTime::StartOfWeek($parse_date(`date`))),
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
        $format_date(Datetime::FromSeconds(CAST(acc_date_entered AS Uint32))) as date_entered,
        ba_id,
        business_segment
    FROM
        `//home/cloud_analytics/kulaga/acc_sales_ba_cube`
);


$ba_first_paid = (
    SELECT 
        DISTINCT billing_account_id,
        $format_date($parse_datetime(first_first_paid_consumption_datetime)) as ba_first_paid_date
    FROM 
        `//home/cloud_analytics/cubes/acquisition_cube/cube`
);

DEFINE SUBQUERY $accounts_raw($period) AS
    SELECT
        `date`,
        --$format_date(DateTime::StartOfWeek($parse_date(`date`))) as date_week,
        $round_period_date($period,`date`) as date_week,
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

DEFINE SUBQUERY $accounts_with_ba($segment,$period) AS
    SELECT  
        date_week,
        count(DISTINCT acc_id) + 0.0 as accounts_with_ba
    FROM 
        $accounts_raw($period)
    WHERE 
        `date` >= date_entered 
        AND business_segment = $segment
        AND ba_first_paid_date is not null
    GROUP BY 
        date_week
END DEFINE;

DEFINE SUBQUERY $accounts_all($segment,$period) AS
    SELECT  
        date_week,
        count(DISTINCT acc_id) + 0.0 as accounts_all
    FROM 
        $accounts_raw($period)
    WHERE 
        `date` >= date_entered 
        AND business_segment = $segment
    GROUP BY 
        date_week
END DEFINE;

DEFINE SUBQUERY $res($segment,$period) AS
SELECT 
    accounts_all.date_week as `date`,
    'crm_territory_coverage_'||if($segment = 'Public sector', 'public_sector', $segment) as metric_id,
    'CRM Territory coverage (' || $segment || ')' as metric_name,
    if($segment != 'Enterprise', 'SMB', 'Enterprise') as metric_group,
    if($segment != 'Enterprise', 'SMB', 'Enterprise') as metric_subgroup,
    '%' as metric_unit, 
    0 as is_ytd,
    $period as period,
    coalesce(accounts_with_ba, 0) / accounts_all * 100.0 as metric_value
FROM 
    $accounts_all($segment,$period) as accounts_all
LEFT JOIN 
    $accounts_with_ba($segment,$period) as accounts_with_ba 
ON accounts_all.date_week = accounts_with_ba.date_week
END DEFINE;

/*
SELECT * FROM 
$metric_growth($res,'Mass','', 'straight','standart','')

UNION ALL

SELECT * FROM 
$metric_growth($res,'Medium','', 'straight','standart','')

UNION ALL

SELECT * FROM 
$metric_growth($res, 'Enterprise','', 'straight','standart','')
*/

    DEFINE SUBQUERY $res_with_metric_growth1($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'Mass', $period, 'straight', 'standart', '')
        )
    END DEFINE;
    DEFINE SUBQUERY $res_with_metric_growth2($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'Medium', $period, 'straight', 'standart', '')
        )
    END DEFINE;
    DEFINE SUBQUERY $res_with_metric_growth3($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'Enterprise', $period, 'straight', 'standart', '')
        )
    END DEFINE;
    DEFINE SUBQUERY $res_with_metric_growth4($period) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, 'Public sector', $period, 'straight', 'standart', '')
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
    PROCESS $s1()
    UNION ALL
    PROCESS $s2()
    UNION ALL
    PROCESS $s3()
    UNION ALL
    PROCESS $s4()

END DEFINE;

EXPORT $crm_territory_coverage;