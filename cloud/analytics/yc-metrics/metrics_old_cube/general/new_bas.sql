/*___DOC___
====KZ New BAs (Companies)
Количество биллинг-аккаунтов-юрлиц для Казахстана (%%ba_person_type='company'%%), зарегистрировавшихся за неделю за исключением фрода (%%is_fraud = 0%%)

<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/marketing/kz_new_bas_companies.sql" formatter="yql"}}
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
            $round_period_datetime($params['period'],event_time) as `date`,
            billing_account_id
        FROM 
            `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE 1=1
            and event = 'ba_created'
            and is_fraud = 0
            and if($params['period'] = 'year', 
                $round_period_datetime($params['period'],event_time) = $round_current_time($params['period']),
                $round_period_datetime($params['period'],event_time) < $round_current_time($params['period']))
            and CASE 
                WHEN $params['mode'] = 'all' then True
                WHEN $params['mode'] = 'company' then ba_person_type = 'company'
                WHEN $params['mode'] = 'kz_companies' then ba_person_type = 'kazakhstan_company'
                WHEN $params['mode'] = 'kz' then ba_person_type in ('kazakhstan_company','kazakhstan_individual')
                WHEN $params['mode'] = 'partner_enterprise' THEN crm_account_segment in ('Enterprise') and is_var = 'var'
                WHEN $params['mode'] = 'partner_SMB' THEN crm_account_segment in ('Medium','Mass') and is_var = 'var' 
                WHEN $params['mode'] = 'partner_public_sector' THEN crm_account_segment in ('Public sector') and is_var = 'var' 
                ELSE True
            END
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