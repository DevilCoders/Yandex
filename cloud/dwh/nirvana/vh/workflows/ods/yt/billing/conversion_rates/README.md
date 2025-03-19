## Conversion rates
#ods #billing #conversion_rates

Вычитывает последний (актуальный) снапшот таблицы (`conversion_rates`)

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                           | Источники                                                                                                                                                              |
|---------|-------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [conversion_rates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/conversion_rates)    | [raw-conversion_rates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/utility/conversion_rates)    |
| PREPROD | [conversion_rates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/conversion_rates) | [raw-conversion_rates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/utility/conversion_rates) |


### Структура
| Поле                  | Описание                                 |
|-----------------------|------------------------------------------|
| start_time_ts         | дата и время начала применения, utc      |
| start_time_dttm_local | дата и время начала применения, local tz |
| multiplier            | множитель                                |
| source_currency       | исходная валюта                          |
| target_currency       | целевая валюта                           |

### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
