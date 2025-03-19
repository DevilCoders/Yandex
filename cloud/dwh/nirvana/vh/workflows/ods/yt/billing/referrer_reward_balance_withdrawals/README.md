## Referrer reward balance withdrawals
#ods #billing #referrer_reward_balance_withdrawals

Вычитывает последний (актуальный) снапшот таблицы (`referrer_reward_balance_withdrawals`)

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                                                                 | Источники                                                                                                                                                                                                 |
|---------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [referrer_reward_balance_withdrawals](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/referrer_reward_balance_withdrawals)    | [raw-referrer_reward_balance_withdrawals](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/meta/referrer_reward_balance_withdrawals)    |
| PREPROD | [referrer_reward_balance_withdrawals](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/referrer_reward_balance_withdrawals) | [raw-referrer_reward_balance_withdrawals](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/meta/referrer_reward_balance_withdrawals) |


### Структура
| Поле               | Описание                       |
|--------------------|--------------------------------|
| amount             | сумма                          |
| balance_client_id  | id баланса клиента             |
| created_ts         | дата время создания, utc       |
| created_dttm_local | дата время создания, local tz  |
| withdrawal_id      | идентификатор вывода средств   |

### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
