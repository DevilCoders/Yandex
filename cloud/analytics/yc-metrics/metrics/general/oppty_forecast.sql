/*___DOC___
==== Oppty Forecast

Метрика показывает ожидаемую выручку по сделкам(оптям) через 3 месяца от начала недели расчета метрики.
Важный моменты:
* в выборку попадают только опти с датой закрытия в течение трёх месяцев, начиная с начала недели даты расчета метрики.
* из выборки исключаются опти в статусах win и loss на начало недели даты расчета метрики
* по факту, если смотреть на метрику в четверг - то она будет отражать картину на момент "понедельник предыдущей недели"

Есть 4 варианта метрики:
1. базовая (pipeline)
2. кумулятивная (pipeline cumulative)
3. взвешенная (forecast weighted)
4. взвешенная кумулятивная (forecast weighted cumulative)

На примере:
Допустим, на сегодня у нас есть только одна открытая сделка, у которой дата закрытия - ровно через месяц, сумма 100р (т.е. ожидаем, что эта оптя будет приносить по 100р в месяц), вероятность 40%. 
Тогда:
* метрика pipeline будет показывать 100р (ожидаем, что оборот по текущим открытым сделкам (которые закроются в течение трёх месяцев) через 3 месяца будет 100р в месяц)
* метрика pipeline cumulative будет показывать 200р (ожидаем, что текущие открытые сделки через 3 месяца суммарно принесут нам 200р с момента их закрытия до момента “через 3 месяца от сейчас”)
* метрика взвешенная (forecast weighted) - будет показывать 40р (100*0.4)
* метрика взвешенная кумулятивная (forecast weighted cumulative) - будет показывать 80р (200*0.4)


<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/general/opty_forecast.sql" formatter="yql"}}
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
        FROM `//home/cloud_analytics/yc-metrics/tmp/opts_forecast`
        WHERE 
            $format_date(DateTime::ShiftMonths($parse_date(`date`),3)) >= date_closed
            AND `date` < date_closed
            AND sales_stage not in ('win', 'loss')
            AND `date` < $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
            AND DateTime::GetDayOfWeekName($parse_date(`date`)) = 'Monday'
            AND CASE
                WHEN $params['segment'] = 'SMB' then segment in ('Mass','Medium')
                ELSE segment = coalesce($params['segment'], '')
            END
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
                        'Mass', 
                        'SMB',
                        'Public sector'
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