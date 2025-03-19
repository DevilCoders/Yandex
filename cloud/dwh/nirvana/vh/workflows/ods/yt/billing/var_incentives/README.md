## Var incentives
#ods #billing #var_incentives

Партнерские премии.
Вычитывает последний (актуальный) снапшот таблицы (`var_incentives`)

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                       | Источники                                                                                                                                                       |
|---------|---------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [var_incentives](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/var_incentives)    | [raw-var_incentives](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/meta/var_incentives)    |
| PREPROD | [var_incentives](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/var_incentives) | [raw-var_incentives](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/meta/var_incentives) |


### Структура
| Поле                                         | Описание                                                              |
|----------------------------------------------|-----------------------------------------------------------------------|
| aggregation_info_interval                    | агрегированная информация, интервал                                   |
| aggregation_info_level                       | агрегированная информация, уровень                                    |
| created_ts                                   | дата и время создания записи, utc                                     |
| created_dttm_local                           | дата и время создания записи, local tz                                |
| default_subaccount_bindings_start_ts         | привязки субаккаунта по умолчанию дата время начала, utc              |
| default_subaccount_bindings_start_dttm_local | привязки субаккаунта по умолчанию дата время начала, local tz         |
| default_subaccount_bindings_end_ts           | привязки субаккаунта по умолчанию дата время завершения, utc          |
| default_subaccount_bindings_end_dttm_local   | привязки субаккаунта по умолчанию дата время завершения, local tz     |
| default_usage_interval_start_ts,             | интервалы использования по умолчанию, дата время начала, utc          |
| default_usage_interval_start_dttm_local,     | интервалы использования по умолчанию, дата время начала, local tz     |
| default_usage_interval_end_ts,               | интервалы использования по умолчанию, дата время завершения, utc      |
| default_usage_interval_end_dttm_local        | интервалы использования по умолчанию, дата время завершения, local tz |
| start_ts                                     | дата время начала, utc                                                |
| start_dttm_local                             | дата время начала, local tz                                           |
| end_ts                                       | дата время завершения, utc                                            |
| end_dttm_local                               | дата время завершения, local tz                                       |
| filter_info_category                         | информация о фильтрах, категория                                      |
| filter_info_entity_ids                       | информация о фильтрах, идентификаторы объектов                        |
| filter_info_level                            | информация о фильтрах, уровень                                        |
| filter_category                              | фильтр, категория                                                     |
| filter_entity_id                             | фильтр, идентификатор объекта                                         |
| filter_level                                 | фильтр, уровень                                                       |
| var_incentive_id                             | id стимула                                                            |
| master_account_id                            | id мастер аккаунта                                                    |
| reward_threshold_multiplier                  | пороговые значения вознаграждения, множитель                          |
| reward_threshold_start_amount                | пороговые значения вознаграждения, начальная сумма                    |
| source_id                                    | id источника                                                          |
| source_type                                  | тип источника                                                         |
| subaccount_binding_start_ts                  | субаккаунт биллинга, время вступления в силу, utc                     |
| subaccount_binding_start_dttm_local          | субаккаунт биллинга, время вступления в силу, local tz                |
| subaccount_binding_end_ts                    | субаккаунт биллинга, дата время истечение, utc                        |
| subaccount_binding_end_dttm_loca             | субаккаунт биллинга, дата время истечение, local tz                   |
| subaccount_binding_id                        | id субаккаунт биллинга                                                |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
