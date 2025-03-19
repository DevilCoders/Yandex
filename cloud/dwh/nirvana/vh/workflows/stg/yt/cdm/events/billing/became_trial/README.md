## Billing events. Became trial
#stg #events #billing

События became_trial создается на основании первого перехода в usage_status 'trial' в billing_accounts_history
*Таблица не рекомендуется к использованию, так как является временной, как часть сборки [витрины событий](../../../../../../cdm/yt/dm_events/README.md)*


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур    | Расположение данных                                                                                                        | Источники                                                                                                                                                                                             |
| --------- |----------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD      | [became_trial](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/stg/cdm/events/billing/billing_account_became_trial)              | [raw-billing_accounts_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/billing_accounts_history)       |
| PREPROD   | [became_trial](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/stg/cdm/events/billing/billing_account_became_trial) | [raw-billing_accounts_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/billing_accounts_history) |


### Структура
| Поле                 | Описание                                                                                                                                 |
|----------------------|------------------------------------------------------------------------------------------------------------------------------------------|
| event_type           | тип события: <br/>billing_account_became_trial - became trial                                                                            |
| event_timestamp      | время события в формате unix timestamp                                                                                                   |
| event_id             | уникальный идентификатор конкретного события                                                                                             |
| event_entity_id      | идентификатор источника события.<br/> * идентификатор биллинг аккаунта                                                                   |
| event_entity_type    | тип источника события. <br/> * `billing_account` - биллинг аккаунт                                                                       |
| event_group          | группа событий. <br/> * `billing` - события биллинга                                                                                     |
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
