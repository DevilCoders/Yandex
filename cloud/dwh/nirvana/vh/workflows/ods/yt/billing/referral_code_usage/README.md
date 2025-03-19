## Referral code usage
#ods #billing #referral_code_usage

Вычитывает последний (актуальный) снапшот таблицы (`referral_code_usage`)

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                                 | Источники                                                                                                                                                                 |
|---------|-------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [referral_code_usage](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/referral_code_usage)    | [raw-referral_code_usage](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/meta/referral_code_usage)    |
| PREPROD | [referral_code_usage](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/referral_code_usage) | [raw-referral_code_usage](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/meta/referral_code_usage) |


### Структура
| Поле                | Описание                       |
|---------------------|--------------------------------|
| activated_ts        | дата время активации, utc      |
| activated_dttm_loca | дата время активации, local tz |
| created_ts          | дата время создания, utc       |
| created_dttm_local  | дата время создания, local tz  |
| referral_code_id    | id                             |
| referral_id         | id referral                    |
| referrer_id         | id referrer                    |

### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
