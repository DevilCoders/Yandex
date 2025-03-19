## Referrer reports
#ods #billing #referrer_reports

Вычитывает последний (актуальный) снапшот таблицы (`referrer_reports`)

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                           | Источники                                                                                                                                                              |
|---------|-------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [referrer_reports](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/referrer_reports)    | [raw-referrer_reports](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/reports/referrer_reports)    |
| PREPROD | [referrer_reports](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/referrer_reports) | [raw-referrer_reports](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/reports/referrer_reports) |


### Структура
| Поле               | Описание                        |
|--------------------|---------------------------------|
| act_month          | месяц                           |
 | referral_id        | id referral                     |
 | referrer_id        | id referrer                     |
 | reward             | вознаграждение, сумма           |
 | reward_month       | вознаграждение, месяц           |
 | updated_ts         | дата время обновления, utc      |
 | updated_dttm_local | дата время обновления, local tz |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
