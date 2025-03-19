## Revenue reports
#ods #billing #revenue_reports

Вычитывает последний (актуальный) снапшот таблицы (`revenue_reports`)

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                         | Источники                                                                                                                                                                     |
|---------|-----------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [revenue_reports](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/revenue_reports)    | [raw-revenue_reports](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/reports/realtime/revenue_reports)    |
| PREPROD | [revenue_reports](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/revenue_reports) | [raw-revenue_reports](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/reports/realtime/revenue_reports) |


### Структура
| Поле                        | Описание                                                                                                                             |
|-----------------------------|--------------------------------------------------------------------------------------------------------------------------------------|
| billing_account_id          | идентификатор [платежного аккаунта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts) |
| created_ts                  | дата и время создания записи, utc                                                                                                    |
| created_dttm_local          | дата и время создания записи, local tz                                                                                               |
| `date`                      | Дата                                                                                                                                 |
| publisher_account_id        | id publisher аккаунта                                                                                                                |
| publisher_balance_client_id | id баланса клиента                                                                                                                   |
| publisher_currency          | валюта                                                                                                                               |
| sku_id                      | id   [продукта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/skus)                                 |
| total                       | сумма                                                                                                                                |

### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
