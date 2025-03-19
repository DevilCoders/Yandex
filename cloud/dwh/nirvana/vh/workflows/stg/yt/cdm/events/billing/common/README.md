#### Billing events. Common

Основные события биллинг аккаунта.

*Таблица не рекомендуется к использованию, так как является временной, как часть сборки [витрины событий](../../../../../../cdm/yt/dm_events/README.md)*

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/stg/cdm/events/billing/billing_account_common)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/stg/cdm/events/billing/billing_account_common)

* `event_type` - тип события
    * `billing_account_created` - событие создания аккаунта
    * `billing_account_became_paid` - событие перехода в платное
    * `billing_account_became_disabled` - событие перехода в disabled состояние
* `event_timestamp` - время события в формате unix timestamp
* `event_id` - уникальный идентификатор конкретного события
* `event_entity_id` - идентификатор источника события.
    * идентификатор биллинг аккаунта
* `event_entity_type` - тип источника события.
    * `billing_account` - биллинг аккаунт
* `event_group` - группа событий.
    * `billing` - события биллинга
* `billing_account_id` - billing account id
* `msk_event_dt` - дата события в iso-формате `%Y-%m-%d` (например, `2021-12-31`) в таймзоне `Europe/Moscow`
* `msk_event_dttm` - дата события в формате `%Y-%m-%d %H:%M:%S` (например, `2021-12-31 23:59:59`) в таймзоне `Europe/Moscow`
* `msk_event_hour` - дата события с округлением до начала часа в iso-формате `%Y-%m-%dT%H:00:00` (например, `2021-12-31T23:00:00`) в таймзоне `Europe/Moscow`
* `msk_event_month_name` - название месяца события (например, `December`) в таймзоне `Europe/Moscow`
* `msk_event_quarter` - квартал события (например, `2021-Q4`) в таймзоне `Europe/Moscow`
* `msk_event_half_year` - полугодие события (например, `2021-H2`) в таймзоне `Europe/Moscow`
* `msk_event_year` - год события (например, `2021`) в таймзоне `Europe/Moscow`
