## Subscription to committed use discount
#billing #subscription_to_committed_use_discount

Вычитывает последний (актуальный) снапшот таблицы связей между подписками и резервом (`subscription_to_committed_use_discount`)


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                                                       | Источники                                                                                                                                                                                                       |
|---------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [subscription_to_committed_use_discount](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/subscription_to_committed_use_discount)    | [raw-subscription_to_committed_use_discount](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/meta/subscription_to_committed_use_discount)    |
| PREPROD | [subscription_to_committed_use_discount](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/subscription_to_committed_use_discount) | [raw-subscription_to_committed_use_discount](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/meta/subscription_to_committed_use_discount) |


### Структура

| Поле                      | Описание                                                           |
|---------------------------|--------------------------------------------------------------------|
| committed_use_discount_id | идентификатор [скидки за резерв](../committed_use_discounts), `FK` |
| subscription_id           | идентификатор [подписки](../subscriptions), `FK`                   |
| __dummy                   |                                                                    |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
