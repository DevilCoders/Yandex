## Billing accounts
#crm #billing_accounts

Таблица связывает аккаунты crm и биллиинга.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                       | Источники                                                                                                                                    |
|---------|---------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/billing_accounts)    | [raw-billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_billingaccounts) |
| PREPROD | [billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/billing_accounts) | [raw-billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_billingaccounts) |


### Структура

| Поле                     | Описание                                                                                                                                      |
|--------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------|
| crm_billing_account_id   | id биллинг аккаунта в crm PK                                                                                                                  |
| crm_account_id           | id [аккаунта в CRM](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_accounts), `FK`                            |
| billing_account_id       | id связанного с ним [биллинг аккаунта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts), `FK` |
| deleted                  | был ли аккаунт удален                                                                                                                         |
| date_modified_ts         | дата время модификации, utc                                                                                                                   |
| date_modified_dttm_local | дата время модификации, local tz                                                                                                              |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
