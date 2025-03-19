## Grant policies
#ods #billing #grant_policies

Вычитывает последний (актуальный) снапшот таблицы (`grant_policies`)

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                       | Источники                                                                                                                                                       |
|---------|---------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [grant_policies](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/grant_policies)    | [raw-grant_policies](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/meta/grant_policies)    |
| PREPROD | [grant_policies](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/grant_policies) | [raw-grant_policies](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/meta/grant_policies) |


### Структура
| Поле                  | Описание                                 |
|-----------------------|------------------------------------------|
| aggregation_span      | диапазон агрегирования                   |
| created_ts            | дата и время создания, utc               |
| created_dttm_local    | дата и время создания, local tz          |
| currency              | валюта                                   |
| deleted_ts            | дата и время удаления, utc               |
| deleted_dttm_local    | дата и время удаления, local tz          |
| end_ts                | дата и время завершения, utc             |
| end_dttm_local        | дата и время завершения, local tz        |
| grant_duration        | срок действия гранта                     |
| grant_policy_id       | id  grant policy                         |
| grant_policy_name     | название  grant policy                   |
| max_amount            | максимальная сумма                       |
| max_count             | максимальное количество                  |
| paid_end_ts           | дата и время завершения оплаты, utc      |
| paid_end_dttm_local   | дата и время завершения оплаты, local tz |
| paid_start_ts         | дата и время начала оплаты, utc          |
| paid_start_dttm_local | дата и время начала оплаты, local tz     |
| rate                  | ставка                                   |
| start_ts              | дата и время создания, utc               |
| start_dttm_local      | дата и время создания, local tz          |
| state                 | состояние                                |

### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
