## Budgeter logs
#ods #billing #budgeter_logs

Вычитывает последний (актуальный) снапшот таблицы (`budgeter_logs`)

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                     | Источники                                                                                                                                                              |
|---------|-------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [budgeter_logs](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/budgeter_logs)    | [raw-budgeter_logs](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/logs/realtime/budgeter_logs)    |
| PREPROD | [budgeter_logs](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/budgeter_logs) | [raw-budgeter_logs](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/logs/realtime/budgeter_logs) |


### Структура
| Поле                      | Описание                                                                                                                             |
|---------------------------|--------------------------------------------------------------------------------------------------------------------------------------|
| billing_account_id        | идентификатор [платежного аккаунта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts) |
| budget_id                 | id budget                                                                                                                            |
| period_id                 | id периода                                                                                                                           |
| period_amount             | период, сумма                                                                                                                        |
| period_execution_time     | период, время выполнения                                                                                                             |
| period_row_count          | период, количество строк                                                                                                             |
| period_updated_ts         | период, дата время обновления, utc                                                                                                   |
| period_updated_dttm_local | период, дата время обновления, local tz                                                                                              |
| shard_id                  | id shard                                                                                                                             |
| state                     | состояние                                                                                                                            |
| updated_ts                | дата время обновления, utc                                                                                                           |
| updated_dttm_local        | дата время обновления, local tz                                                                                                      |

### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
