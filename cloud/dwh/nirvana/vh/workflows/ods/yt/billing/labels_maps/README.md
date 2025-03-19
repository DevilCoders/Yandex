## Labels maps
#ods #billing #labels_maps

Вычитывает последний (актуальный) снапшот таблицы (`labels_maps`)

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                         | Источники                                                                                                                                                         |
|---------|-----------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [labels_maps](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/labels_maps)    | [raw-labels_maps](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/meta/labels_maps)    |
| PREPROD | [labels_maps](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/labels_maps) | [raw-labels_maps](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/meta/labels_maps) |


### Структура
| Поле               | Описание                                                                                                                             |
|--------------------|--------------------------------------------------------------------------------------------------------------------------------------|
| billing_account_id | идентификатор [платежного аккаунта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts) |
| date               | дата                                                                                                                                 |
| amount             | сумма                                                                                                                                |
| created_by         | создано                                                                                                                              |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
