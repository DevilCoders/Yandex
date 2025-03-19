## Service accounts
#ods #yt #iam #subjects #service_accounts #PII

Таблица сервисных аккаунтов.

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                                       | Расположение данных-PII                                                                                                           | Источники                                                                                                                                                                                         |
|---------|-------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [service_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/iam/service_accounts)    | [service_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/iam/service_accounts)    | [subjects/service_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/hardware/default/identity/r3/subjects/service_accounts), [subjects/service_accounts_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/hardware/default/identity/r3/subjects/service_accounts_history)  |
| PREPROD | [service_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/service_accounts) | [service_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/service_accounts) | [subjects/service_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/identity/hardware/default/identity/r3/subjects/service_accounts), [subjects/service_accounts_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/hardware/default/identity/r3/subjects/service_accounts_history) |


### Структура
| Поле                                 | Описание                                                             |
|--------------------------------------|----------------------------------------------------------------------|
| iam_service_account_id               | Идентификатор сервисного аккаунта                                    |
| iam_service_account_status           | статус                                                               |
| iam_service_account_description_hash | Описание, hash                                                       |
| created_ts                           | Дата создания UTC                                                    |
| created_dttm_local                   | Дата создания MSK                                                    |
| modified_ts                          | Дата изменения UTC                                                   |
| modified_dttm_local                  | Дата изменения MSK                                                   |
| deleted_ts                           | Восстановленная из исторической таблицы дата удаления субъекта в UTC |
| deleted_dttm_local                   | Дата удаления субъекта в MSK                                         |
| iam_cloud_id                         | Идентификатор облака                                                 |
| iam_folder_id                        | Идентификатор папки                                                  |
| iam_labels                           | labels                                                               |
| login_hash                           | Логин, hash                                                          |


### Структура PII
| Поле                            | Описание                          |
|---------------------------------|-----------------------------------|
| iam_service_account_id          | Идентификатор сервисного аккаунта |
| iam_service_account_description | Описание                          |
| login                           | Логин                             |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
