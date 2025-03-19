/*___DOC___
====Conversion Trial -> Paid
Средняя конверсия Trial->Paid за последние 10 недель. Выбирается кол-во конверсий Trial -> Paid за неделю среди биллинг-аккаунтов начавших триал не более чем за 10 недель до недели расчета метрики(биллинг-аккаунты, начавшие платное потребление без триала не учитываются). Это кол-во делится на общее кол-во биллинг-аккаунтов начавших триал не болле чем за 10 недель до недели расчета метрики и умножается на 10.
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/SMB/conversion_trial_paid.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_from_second, $format_datetime,
 $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

$format_name = ($name) -> { RETURN Unicode::ToLower(CAST(coalesce($name,'') as Utf8)) };
$replace = Re2::Replace(" ");

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

DEFINE SUBQUERY $conversion_trial_paid() AS
    
    DEFINE SUBQUERY $filters($params) AS 
        SELECT
            billing_account_id
        FROM 
            `//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags`
        WHERE 
            cast(`date` as Date) = CurrentUtcDate()
            and is_suspended_by_antifraud_current = False
            and CASE
                WHEN $params['mode'] = 'company' then person_type_current = 'company'
                else True
            END
    END DEFINE;

    DEFINE SUBQUERY $ba_trial($params) AS 
        SELECT
            $format_from_second(event_timestamp) as first_trial,
            event_entity_id as ba_trial
        FROM `//home/cloud-dwh/data/prod/cdm/dm_events`
        WHERE event_type = 'billing_account_first_trial_consumption'
    END DEFINE;

    DEFINE SUBQUERY $ba_paid($params) AS 
        SELECT
            $format_from_second(event_timestamp) as first_paid,
            event_entity_id as ba_paid
        FROM `//home/cloud-dwh/data/prod/cdm/dm_events`
        WHERE event_type = 'billing_account_first_paid_consumption'
    END DEFINE;

    DEFINE SUBQUERY $res($params) AS
        SELECT
            $round_period_datetime($params['period'],$format_datetime(first_trial + INTERVAL('P70D'))) as `date`,
            if($params['period'] = 'week' and $format_date(first_paid) < $format_date(first_trial + INTERVAL('P70D')),
                1,
                if($params['period'] = 'month' and $format_date(first_paid) < $format_date(first_trial + INTERVAL('P70D')),
                    1,
                    0
                )
            ) as ba_conv,
            ba_trial
        FROM $ba_trial($params) as trial
        LEFT JOIN $ba_paid($params) as paid 
        ON trial.ba_trial = paid.ba_paid
        JOIN $filters($params) as filters
        ON trial.ba_trial = filters.billing_account_id
        WHERE 
            first_trial <= coalesce(first_paid,cast('2099-01-01' as Date))
            and first_trial + INTERVAL('P70D')  < CurrentUtcDate()
    END DEFINE;

    DEFINE SUBQUERY $res($params) as 
        SELECT 
            `date`,
            'conversion_trial_paid_'||$format_name($params['mode']) as metric_id,
            'Conversion Trial => Paid '||$format_name($params['mode']) as metric_name,
            'marketing' as metric_group,
            'marketing' as metric_subgroup,
            '%' as metric_unit, 
            0 as is_ytd,
            cast($params['period'] as Utf8) as period,
            sum(ba_conv)/(count(distinct ba_trial) + 0.0) * 100 as metric_value
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
        $make_param_dict(
            $product(
                AsList(
                    AsList(
                        'week',
                        'month' 
                    ), 
                    AsList(
                        'all',
                        'company'
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()


END DEFINE;

EXPORT $conversion_trial_paid;