## Data Mart Events
#dm #events

Витрина событий Облака. Собирается из конкатенации отдельных таблиц из stg-слоя:

* [billing.common](../../../stg/yt/cdm/events/billing/common/README.md)
* [billing.feature_flag_changed](../../../stg/yt/cdm/events/billing/feature_flag_changed/README.md)
* [billing.first_paid_consumption](../../../stg/yt/cdm/events/billing/first_paid_consumption/README.md)
* [billing.first_payment](../../../stg/yt/cdm/events/billing/first_payment/README.md)
* [billing.first_trial_consumption](../../../stg/yt/cdm/events/billing/first_trial_consumption/README.md)
* [billing.state_changed](../../../stg/yt/cdm/events/billing/state_changed/README.md)
* [iam.cloud_created](../../../stg/yt/cdm/events/iam/cloud_created/README.md)
* [billing.first_service_consumption](../../../stg/yt/cdm/events/billing/first_service_consumption/README.md)
* [iam.cloud_created](../../../stg/yt/cdm/events/iam/create_organization/README.md)

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур    | Расположение данных                                                                                         |
| --------- |-------------------------------------------------------------------------------------------------------------|
| PROD      | [became_trial](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/dm_events)     |
| PREPROD   | [became_trial](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/dm_events)  |


### Структура
| Поле                 | Описание                                                                                                                                 |
|----------------------|------------------------------------------------------------------------------------------------------------------------------------------|
| event_type           | тип события                                                                                                                              |
| event_timestamp      | время события в формате unix timestamp                                                                                                   |
| event_id             | уникальный идентификатор конкретного события                                                                                             |
| event_entity_id      | идентификатор источника события.                                                                                                         |
| event_entity_type    | тип источника события.                                                                                                                   |
| event_group          | группа событий.                                                                                                                          |
| billing_account_id   | [billing account id](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts)                    |
| msk_event_dt         | дата события в iso-формате `%Y-%m-%d` (например, `2021-12-31`) в таймзоне `Europe/Moscow`                                                |
| msk_event_dttm       | дата события в формате `%Y-%m-%d %H:%M:%S` (например, `2021-12-31 23:59:59`) в таймзоне `Europe/Moscow`                                  |
| msk_event_hour       | дата события с округлением до начала часа в iso-формате `%Y-%m-%dT%H:00:00` (например, `2021-12-31T23:00:00`) в таймзоне `Europe/Moscow` |
| msk_event_month_name | название месяца события (например, `December`) в таймзоне `Europe/Moscow`                                                                |
| msk_event_quarter    | квартал события (например, `2021-Q4`) в таймзоне `Europe/Moscow`                                                                         |
| msk_event_half_year  | полугодие события (например, `2021-H2`) в таймзоне `Europe/Moscow`                                                                       |
| msk_event_year       | год события (например, `2021`) в таймзоне `Europe/Moscow`                                                                                |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
