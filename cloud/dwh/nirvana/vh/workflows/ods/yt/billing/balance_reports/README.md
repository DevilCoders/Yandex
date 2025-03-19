## Balance reports
#ods #billing #balance_reports

Вычитывает последний (актуальный) снапшот таблицы (`balance_reports`)

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                         | Источники                                                                                                                                                         |
|---------|-----------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [balance_reports](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/balance_reports)    | [raw-balance_reports](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/reports/realtime/balance_reports)    |
| PREPROD | [balance_reports](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/balance_reports) | [raw-balance_reports](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/reports/realtime/balance_reports) |


### Структура
| Поле               | Описание                                                                                                                                |
|--------------------|-----------------------------------------------------------------------------------------------------------------------------------------|
| balance_product_id | id [продукта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/skus)                                      |
| billing_account_id | идентификатор [платежного аккаунта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts)    |
| created_ts         | дата и время создания, utc                                                                                                              |
| created_dttm_local | дата и время создания, local tz                                                                                                         |
| date               | дата                                                                                                                                    |
| subaccount_id      | идентификатор [платежного субаккаунта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts) |
| total              | сумма                                                                                                                                   |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
