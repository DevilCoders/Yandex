/*___DOC___
====New BAs
Количество биллинг-аккаунтов, зарегистрировавшихся за выбранный период.
Из выборки исключены фродовое (billing_account_is_suspended_by_antifraud на момент рассчитываемого потребления) клиенты.
Клиент считается фродовым по логике, описанной тут: https://st.yandex-team.ru/CLOUD-79491#61485d91ccf5186c3e578fa0
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/new_bas.sql" formatter="yql"}}
}>
___DOC___*/


use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, 
                    $round_period_datetime, $round_period_date,  $round_period_format,
                    $round_current_time;
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


DEFINE SUBQUERY $new_bas() AS
    DEFINE SUBQUERY $res($params) AS
        SELECT 
            `date`,
            'new_ba_'||$format_name($params['mode']) as metric_id,
            'New BAs '||CAST($params['mode'] as Utf8) as metric_name,
            'general' as metric_group,
            'general' as metric_subgroup,
            '' as metric_unit, 
            0 as is_ytd,
            CAST($params['period'] as Utf8) as period,
            count(distinct billing_account_id) as metric_value
        FROM (
        SELECT 
            $round_period_format($params['period'],`date`) as `date`,
            billing_account_id as billing_account_id
        FROM    
            (
            SELECT 
                hist.billing_account_id as billing_account_id, 
                event_dt as `date`
            FROM 
                `//home/cloud-dwh/data/prod/cdm/dm_ba_history` as hist
            LEFT JOIN `//home/cloud-dwh/data/prod/ods/billing/billing_accounts` as ba
            ON hist.billing_account_id = ba.billing_account_id
            WHERE 
                if($params['period'] = 'year', 
                    $round_period_format($params['period'],event_dt) = $round_current_time($params['period']),
                    $round_period_format($params['period'],event_dt) < $round_current_time($params['period']))
                and CASE 
                    WHEN $params['mode'] = 'all' then True
                    WHEN $params['mode'] = 'company' then billing_account_type = 'company'
                    WHEN $params['mode'] = 'kz_companies' then billing_account_type = 'company' AND yandex_office = 'kazakhstan'
                    WHEN $params['mode'] = 'kz' then yandex_office = 'kazakhstan'
                    WHEN $params['mode'] = 'partner_enterprise' THEN crm_segment in ('Enterprise') and (hist.is_var = True or hist.is_subaccount = True)
                    WHEN $params['mode'] = 'partner_SMB' THEN crm_segment in ('Medium','Mass') and (hist.is_var = True or hist.is_subaccount = True)
                    WHEN $params['mode'] = 'partner_public_sector' THEN crm_segment in ('Public sector') and (hist.is_var = True or hist.is_subaccount = True)
                    ELSE True
                END
                and ba.is_suspended_by_antifraud = False
                and action = 'create'
            ) as events
        )
        GROUP BY `date`
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
                        'all',
                        'company',
                        'kz',
                        'kz_companies',
                        'partner_enterprise',
                        'partner_SMB',
                        'partner_public_sector'
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $new_bas;