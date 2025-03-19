## crm leads billing accounts
#crm #crm_leads_billing_accounts

Таблица связывает платежный аккаунт и лиды.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур    | Расположение данных   | Источники |
| --------- | -------------------   | --------- |
| PROD      | [crm_leads_billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_leads_billing_accounts) | [raw-crm_leads_billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_leads_billing_accounts) |
| PREPROD   | [crm_leads_billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_leads_billing_accounts) | [raw-crm_leads_billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_leads_billing_accounts) |


### Структура

| Поле                     | Описание                                                                                                                    |
|--------------------------|-----------------------------------------------------------------------------------------------------------------------------|
| crm_billing_accounts_id  | id [платежного аккаунта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/billing_accounts), `FK` |
| date_modified_ts         | дата время модификации, utc                                                                                                 |
| date_modified_dttm_local | дата время модификацииа, local tz                                                                                           |
| deleted                  | была ли удалена                                                                                                             |
| id                       | id                                                                                                                          |
| crm_leads_id             | id [лида](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_leads), `FK`                       |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
