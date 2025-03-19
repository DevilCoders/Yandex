## Volume incentives
#ods #billing #volume_incentives

Вычитывает последний (актуальный) снапшот таблицы (`volume_incentives`)

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                             | Источники                                                                                                                                                             |
|---------|---------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [volume_incentives](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/volume_incentives)    | [raw-volume_incentives](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/meta/volume_incentives)    |
| PREPROD | [volume_incentives](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/volume_incentives) | [raw-volume_incentives](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/meta/volume_incentives) |


### Структура
| Поле                      | Описание                                                                                                                             |
|---------------------------|--------------------------------------------------------------------------------------------------------------------------------------|
| aggregation_info          | агрегированная информация                                                                                                            |
| billing_account_id        | идентификатор [платежного аккаунта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts) |
| created_ts                | дата и время создания записи, utc                                                                                                    |
| created_dttm_local        | дата и время создания записи, local tz                                                                                               |
| effective_time_ts         | эффективное время, utc                                                                                                               |
| effective_time_dttm_local | эффективное время, local tz                                                                                                          |
| expiration_time_ts        | дата время истечения, utc                                                                                                            |
| expiration_time_local     | дата время истечения, local tz                                                                                                       |
| filter_info_category      | информация о фильтре, категория                                                                                                      |
| filter_info_entity_ids    | информация о фильтре, ids                                                                                                            |
| filter_info_level         | информация о фильтре, уровень                                                                                                        |
| volume_incentive_id       | id                                                                                                                                   |
| thresholds_multiplier     | порог, умножение                                                                                                                     |
| thresholds_start_amount   | порог, стартовая сумма                                                                                                               |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
