/*___DOC___
====Название метрики
Описание метрики
<{График
{{iframe frameborder="0" width="70%" height="300px" src="ссылка-на-график"}}
}>
<{Код
{{include href="https://a.yandex-team.ru/api/tree/blob/trunk/arcadia/cloud/analytics/yc-metrics/metrics/ссылка-на-метрику" formatter="yql"}}
}>
___DOC___*/

use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date, $format_date, $dates_range, $round_period_datetime, $round_period_date,  $round_current_time;
IMPORT time_series SYMBOLS $metric_growth;

DEFINE SUBQUERY $metric_name() AS -- должно совпадать с именем файла
    DEFINE SUBQUERY $res($param,$period) AS
        SELECT 
            `date`,
            'metric_id' as metric_id, --без пробелов
            'Metric Name' as metric_name, 
            'group' as metric_group, -- должно совпадать с названием группы
            'subgroup' as metric_subgroup,
            '' as metric_unit, 0 as is_ytd,
            $period as period, -- одно из трех ('','%','₽')
            sum(*) as metric_value
        FROM ...
    END DEFINE;

    SELECT * FROM
    $metric_growth($res, '', 'inverse','goal','100') -- "inverse/straight", "goal/standart"


END DEFINE;

EXPORT $metric_name;