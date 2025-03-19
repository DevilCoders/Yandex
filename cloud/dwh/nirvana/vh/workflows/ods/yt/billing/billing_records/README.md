## Billing records
#Billing #records

Часовые агрегаты потребления. Данные разбиты на дневные таблицы

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)

### Расположение данных
| Контур           | Расположение данных                                                                                                                      | Источники                                                                                                                                           |
|------------------|------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD             | [billing_records](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/billing_records/1mo)             | [raw_billing_records](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/logfeller/billing/enriched_metrics)             |
| PREPROD          | [billing_records](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/billing_records/1mo)          | [raw_billing_records](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/logfeller/billing/enriched_metrics)          |
| PROD_INTERNAL    | [billing_records](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/ods/billing/billing_records/1mo)    | [raw_billing_records](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/raw/logfeller/billing/enriched_metrics)    |
| PREPROD_INTERNAL | [billing_records](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/ods/billing/billing_records/1mo) | [raw_billing_records](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/raw/logfeller/billing/enriched_metrics) |


### Структура
| Поле                             | Описание                                                                                                   |
|----------------------------------|------------------------------------------------------------------------------------------------------------|
| billing_account_id               | id биллинг аккаунта                                                                                        |
| cloud_id                         | id облака                                                                                                  |
| folder_id                        | id фолдера                                                                                                 |
| resource_id                      | id ресурса                                                                                                 |
| master_account_id                | id биллинг аккаунта, который является VAR-ом для `billing_account_id`                                      |
| start_time                       | начало часа потребления в формате unix timestamp                                                           |
| end_time                         | конец часа потребления в формате unix timestamp                                                            |
| date                             | дата потребления в iso-формате `%Y-%m-%d` (например, `2021-12-31`) в таймзоне `Europe/Moscow`              |
| month                            | месяц потребления в iso-формате `%Y-%m-01` (например, `2021-12-01`) в таймзоне `Europe/Moscow`             |
| currency                         | валюта                                                                                                     |
| metric_schema                    | схема метрики                                                                                              |
| sku_id                           | id SKU                                                                                                     |
| rate_id                          | id rate для пороговых sku                                                                                  |
| pricing_unit                     | тип единицы потребления для подсчета стоимости                                                             |
| unit_price                       | цена за единицу `pricing_unit`                                                                             |
| sku_overridden                   | флаг, который указывает, был ли применен sku override                                                      |
| tiered_pricing_quantity          | размер потребления в единицах `pricing_unit`, которые были учтены в текущем пороге sku (для пороговых sku) |
| pricing_quantity                 | размер потребления в единицах `pricing_unit`                                                               |
| usage_quantity                   | размер потребления в единицах `usage_unit`                                                                 |
| cost                             | стоимость в валюте `currency`                                                                              |
| credit                           | общая скидка                                                                                               |
| expense                          | итого к оплате с учетом скидок в валюте `currency`                                                         |
| monetary_grant_credit            | скидка от Monetary Grant                                                                                   |
| cud_credit                       | скидка от CUD (Committed Use Discount, CVoS)                                                               |
| cud_compensated_pricing_quantity | размер потребления в единицах `pricing_unit`, которые были покрыты с помощью CUD                           |
| volume_incentive_credit          | скидка от Volume Incentive                                                                                 |
| trial_credit                     | скидка заблокированного триального пользователя                                                            |
| disabled_credit                  | скидка инактивированного аккаунта                                                                          |
| service_credit                   | скидка для сервисных аккаунтов                                                                             |
| rewarded_expense                 | стоимость потребления, от которого посчитано поле `volume_reward`                                          |
| reward                           | var-премия в валюте `currency`                                                                             |
| publisher_account_id             | id паблишера продукта                                                                                      |
| publisher_balance_client_id      | id клиента баланса, который принадлежит паблишеру                                                          |
| publisher_currency               | валюта паблишера                                                                                           |
| publisher_revenue                | премия паблишеру в валюте `publisher_currency`                                                             |
| labels_json                      | json представление поля `labels`                                                                           |
| labels_hash                      | хеш поля `labels`                                                                                          |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: раз в час.
