/*___DOC___
==== Oppty Forecast
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/support/opty_forecast.sql" formatter="yql"}}
}>
___DOC___*/


use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT tables SYMBOLS $last_non_empty_table;
IMPORT time_series SYMBOLS $metric_growth;
IMPORT lib_opt_history SYMBOLS $lib_opt_history;

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
                        AsTuple('mode', coalesce($x[0],'')),
                        AsTuple('segment', coalesce($x[1],'')),
                        AsTuple('period', coalesce($x[2],''))
                    )
                )
            }
        )
};

DEFINE SUBQUERY $oppty_forecast() AS
    DEFINE SUBQUERY $res($params) AS
        SELECT 
            `date`,
            'opty_forecast_' || coalesce($format_name($params['mode']), '') || '_' || $format_name($params['segment']) AS metric_id,
            'Opty Forecast ' || coalesce($format_name($params['mode']), '') || '_' || $format_name($params['segment']) as metric_name,
            'sales' as metric_group,
            'sales' as metric_subgroup,
            '₽' as metric_unit, 
            0 as is_ytd,
            CAST($params['period'] as Utf8) as period,
            CASE  coalesce($params['mode'], '')
                WHEN '' THEN 
                    sum(amount)
                WHEN 'weighted' THEN
                    sum(
                        amount * 
                        COALESCE(CAST(probability AS Double), 0) / 100.0
                    )
                WHEN 'cum' THEN 
                    sum(
                        amount * 
                        Math::Ceil(
                            CAST(
                                DateTime::ToDays(
                                    DateTime::MakeDate(DateTime::ShiftMonths($parse_date(`date`),3)) - 
                                    Datetime::MakeDate($parse_date(date_closed))
                                ) 
                                AS Double
                            ) / 365.0 * 12
                        )
                    )
                WHEN 'weighted_cum' THEN 
                    sum(
                        amount * 
                        COALESCE(CAST(probability AS Double), 0) / 100.0 * 
                        Math::Ceil(
                            CAST(
                                DateTime::ToDays(
                                    DateTime::MakeDate(DateTime::ShiftMonths($parse_date(`date`),3)) - 
                                    Datetime::MakeDate($parse_date(date_closed))
                                ) 
                                AS Double 
                            ) / 365.0 * 12
                        )
                    )
                ELSE 0
            END as metric_value
        -- FROM $lib_opt_history()
        FROM `//home/cloud_analytics/yc-metrics/tmp/opts_forecast`
        WHERE 
            $format_date(DateTime::ShiftMonths($parse_date(`date`),3)) >= date_closed
            AND `date` < date_closed
            AND sales_stage not in ('win', 'loss')
            AND `date` < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
            AND DateTime::GetDayOfWeekName($parse_date(`date`)) = 'Monday'
            AND segment = coalesce($params['segment'], '')
        GROUP BY 
            `date`
        ORDER BY 
            `date` DESC
    END DEFINE;

    DEFINE SUBQUERY $res_with_metric_growth($params) AS
        SELECT *
        FROM 
            $metric_growth($res, $params, 'straight','standart', '')
    END DEFINE;

    $s = SubqueryUnionAllFor(
        $make_param_dict(
            $product(
                AsList(
                    AsList(
                        '', 
                        'weighted',
                        'cum', 
                        'weighted_cum'
                    ), 
                    AsList(
                        'Enterprise',
                        'Medium',
                        'Mass'
                    ),
                    AsList(
                        'week'
                    )
                )
            )
        ),
        $res_with_metric_growth
    );

    PROCESS $s();

END DEFINE;

EXPORT $oppty_forecast;