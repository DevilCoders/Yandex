/*___DOC___
====First Trial Cons
Количество биллинг-аккаунтов, активировавших триал за выбранный период. 
Из выборки исключены фродовое (billing_account_is_suspended_by_antifraud на момент рассчитываемого потребления) клиенты.
Клиент считается фродовым по логике, описанной тут: https://st.yandex-team.ru/CLOUD-79491#61485d91ccf5186c3e578fa0
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/first_trial_cons.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
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
                        AsTuple('segment', coalesce($x[1],''))
                    )
                )
            }
        )
};


DEFINE SUBQUERY $first_trial_cons() AS
    DEFINE SUBQUERY $res($params) AS 
        SELECT 
            `date`,
            'first_trial_cons_'||$format_name($params['segment']) as metric_id,
            'First Trial Cons '||CAST($params['segment'] as Utf8) as metric_name,
            'general' as metric_group,
            'general' as metric_subgroup,
            'BAs' as metric_unit, 
            0 as is_ytd,
            cast($params['period'] as Utf8) as period,
            count(distinct(billing_account_id)) as metric_value
        FROM (
            SELECT 
                $round_period_datetime($params['period'],msk_event_dttm) as `date`,
                cons.billing_account_id as billing_account_id
            FROM 
                `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption` as cons
            JOIN    
                (
                SELECT 
                    billing_account_id,
                    msk_event_dttm
                FROM 
                    `//home/cloud-dwh/data/prod/cdm/dm_events`
                WHERE 
                    event_type = 'billing_account_first_trial_consumption'
                ) as events
            ON 
                cons.billing_account_id = events.billing_account_id
            WHERE 
                CASE
                    WHEN $params['segment'] = 'Enterprise' THEN crm_segment_current in ('Enterprise')
                    WHEN $params['segment'] = 'Public sector' THEN crm_segment_current in ('Public sector')
                    WHEN $params['segment'] = 'SMB' THEN crm_segment_current in ('Mass', 'Medium')
                    ELSE True
                END 
                and billing_account_is_suspended_by_antifraud = False
                and if($params['period'] = 'year', 
                    $round_period_datetime($params['period'],msk_event_dttm) = $round_current_time($params['period']),
                    $round_period_datetime($params['period'],msk_event_dttm) < $round_current_time($params['period'])
                )
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
                        'Enterprise',
                        'Public sector',
                        'SMB'
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $first_trial_cons;