## crm accounts opportunities
#crm #crm_accounts_opportunities

Таблица связывает crm аккаунт и сделку.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                        | Источники                                                                                                                                                     |
|---------|--------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_accounts_opportunities](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_accounts_opportunities) | [raw-crm_accounts_opportunities](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_accounts_opportunities) |
| PREPROD | [crm_accounts_opportunities](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_accounts_opportunities) | [raw-crm_accounts_opportunities](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_accounts_opportunities) |


### Структура

| Поле                     | Описание                                                                                                           |
|--------------------------|--------------------------------------------------------------------------------------------------------------------|
| crm_account_id           | id [аккаунта в CRM](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_accounts), `FK` |
| date_modified_ts         | дата время изменения, utc                                                                                          |
| date_modified_dttm_local | дата время изменения, local tz                                                                                     |
| deleted                  | была ли строка удалена                                                                                             |
| id                       | id                                                                                                                 |
| crm_opportunity_id       | id [сделки](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_opportunities), `FK`    |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
