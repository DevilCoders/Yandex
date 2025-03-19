/*___DOC___
==== Tickets Created Per 100 BA
Число тикетов, созданных на 100 активных (имеющих триальное или платное потребление за вычетом sku_lazy = 1) биллинга аккаунтов в выбранных период в разбивке на на сегменты / статусы биллинг аккаунта / тип поддержки. 
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/tickets_created_per_ba.sql" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;
IMPORT lib_tickets_created SYMBOLS $lib_tickets_created;
IMPORT lib_ba_count SYMBOLS $lib_ba_count;


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


DEFINE SUBQUERY $tickets_created_per_ba() AS 
    DEFINE SUBQUERY $res($params) AS 
        SELECT 
            tickets_created.`date` as `date`,
            $format_name($params['mode']) as metric_id,
            $format_name($params['mode']) as metric_name,
            'support' as metric_group,
            'support' as metric_subgroup,
            '' as metric_unit, 
            0 as is_ytd,
            CAST($params['period'] as Utf8) as period,
            (tickets_count + 0.0) / (ba_count) * 100 as metric_value
        FROM $lib_tickets_created($params) as tickets_created
        LEFT JOIN $lib_ba_count($params) as ba_count 
        ON tickets_created.`date` = ba_count.`date`
    END DEFINE; 

    DEFINE SUBQUERY $res_with_metric_growth($params) AS (
        SELECT 
            *
        FROM 
            $metric_growth($res, $params, 'inverse', 'standart', '')
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
                        'support_tickets_count_per_ba_all',
                        'support_tickets_count_per_ba_ba_paid_status_paid',
                        'support_tickets_count_per_ba_ba_paid_status_trial', 
                        'support_tickets_count_per_ba_segment_mass', 
                        'support_tickets_count_per_ba_segment_medium', 
                        'support_tickets_count_per_ba_segment_enterprise', 
                        'support_tickets_count_per_ba_support_type_free', 
                        'support_tickets_count_per_ba_support_type_allpaid', 
                        'support_tickets_count_per_ba_support_type_standard',
                        'support_tickets_count_per_ba_support_type_business',
                        'support_tickets_count_per_ba_support_type_premium' 
                    )
                )
            )
        ),
        $res_with_metric_growth
        );

    PROCESS $s()

END DEFINE;

EXPORT $tickets_created_per_ba;

