## Budgets
#ods #billing #budgets

Вычитывает последний (актуальный) снапшот таблицы (`budgets`)

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                         | Источники                                                                                                                                         |
|---------|-------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [budgets](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/budgets)    | [raw-budgets](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/meta/budgets)    |
| PREPROD | [budgets](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/budgets) | [raw-budgets](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/meta/budgets) |


### Структура
| Поле                              | Описание                                                                                                                             |
|-----------------------------------|--------------------------------------------------------------------------------------------------------------------------------------|
| billing_account_id                | идентификатор [платежного аккаунта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts) |
| budget_id                         | id бюджета                                                                                                                           |
| budget_name                       | название бюджета                                                                                                                     |
| budget_type                       | тип бюджета                                                                                                                          |
| budgeted_amount                   | сумма бюджет                                                                                                                         |
| created_by                        | создано                                                                                                                              |
| created_ts                        | дата время создания, utc                                                                                                             |
| created_dttm_local                | дата время создания, local tz                                                                                                        |
| end_date_dttm_local               | дата завершения, local tz                                                                                                            |
| end_date_ts                       | дата завершения, utc                                                                                                                 |
| filter_info_cloud_and_folder_ids  | информация о фильтре, идентификаторы облака и папок                                                                                  |
| filter_info_service_id            | информация о фильтре, id [сервиса](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/services)          |
| notification_user                 | уведомление для пользователей                                                                                                        |
| reset_period                      | период сброса                                                                                                                        |
| start_date_dttm_local             | дата время начала, local tz                                                                                                          |
| start_date_ts                     | дата время начала, utc                                                                                                               |
| state                             | состояние                                                                                                                            |
| threshold_rule_notification_users | пороговое правило, пороговое уведомление пользователей                                                                               |
| threshold_rule_threshold          | пороговое правило, пороговое значение                                                                                                |
| threshold_rule_threshold_amount   | пороговое правило, пороговая сумма                                                                                                   |
| threshold_rule_threshold_type     | пороговое правило, пороговый тип                                                                                                     |
| updated_dttm_local                | дата время обновления, local tz                                                                                                      |
| updated_ts                        | дата время обновления, utc                                                                                                           |
| use_in_cloud_function             | использование cloud_function?                                                                                                        |

### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
