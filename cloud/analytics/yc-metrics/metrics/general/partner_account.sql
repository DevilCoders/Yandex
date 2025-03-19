/*___DOC___
====Partner Account
Число активных пользователей за выбранный период.
Активным пользователем считается Billing Account, по которому было платное осознанное (sku_lazy = 0) потребление в течение этого периода.
Из выборки исключены фродовое (billing_account_is_suspended_by_antifraud на момент рассчитываемого потребления) клиенты.
Клиент считается фродовым по логике, описанной тут: https://st.yandex-team.ru/CLOUD-79491#61485d91ccf5186c3e578fa0

В качестве порога берется сумма платного потребления Партнера (вместе с Сабаккаунтами) в 1 000 000 р в месяц.

Partner_account считается как coalesce(billing_master_account_id,billing_account_id) для (billing_account_is_var = True or billing_account_is_subaccount = True)


<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/partner_account.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
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



DEFINE SUBQUERY $partner_account() AS
    DEFINE SUBQUERY $res($params) AS 
        SELECT 
            `date`,
            'ba_active'||'_'||$format_name($params['mode']) as metric_id,
            'BA active'||'_'||CAST($params['mode'] as Utf8)  as metric_name,
            'general' as metric_group,
            'general' as metric_subgroup,
            'BAs' as metric_unit, 
            0 as is_ytd,
            CAST($params['period'] AS Utf8) as period,
            count(DISTINCT partner_account) as metric_value
        FROM (
            SELECT 
                `date`,
                partner_account,
                sum(cons) as cons
            FROM (
                SELECT 
                    $round_period_date($params['period'], billing_record_msk_date) as `date`,
                    coalesce(billing_master_account_id,billing_account_id) as partner_account,
                    billing_record_real_consumption_rub_vat as cons
                FROM 
                    `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption` 
                WHERE 
                    if($params['period'] = 'year',
                        $round_period_date($params['period'],billing_record_msk_date) = $round_current_time($params['period']),
                        $round_period_date($params['period'],billing_record_msk_date) < $round_current_time($params['period']))
                    and sku_lazy = 0
                    and billing_account_is_suspended_by_antifraud = False
                    and billing_record_real_consumption_rub_vat > 0
                    and (billing_account_is_var = True or billing_account_is_subaccount = True)
            )
            GROUP BY 
                `date`,
                partner_account       
        )
        WHERE 
            CASE
                WHEN $params['mode'] = 'partner_before_treshhold' then cons < 1000000
                WHEN $params['mode'] = 'partner_after_treshhold' then cons >= 1000000
                else True
            END 
        GROUP BY `date`
    END DEFINE;


    DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $params, 'straight', 'growth', '')
        )
    END DEFINE;


    $s = SubqueryUnionAllFor(
        $make_param_dict(
            $product(
                AsList(
                    AsList(
                        'month'
                    ), 
                    AsList(
                        'partner_before_treshhold',
                        'partner_after_treshhold'
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $partner_account;