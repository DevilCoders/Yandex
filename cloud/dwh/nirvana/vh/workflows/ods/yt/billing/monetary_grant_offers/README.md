#### Описание

Вычитывает последний (актуальный) снапшот таблицы `monetary_grant_offers`

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/monetary_grant_offers)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/monetary_grant_offers)

* `monetary_grant_offer_id` - идентификатор промокода
* `created_at_msk`          - момент создания промокода
* `duration`                - длительность (в секундах) гранта после активации промокода
* `initial_amount`          - сумма гранта (в валюте `currency`) в случае активации
* `currency`                - валюта поля `initial_amount`
* `passport_user_id`        -
* `proposed_to`             -
* `reason`                  - причина создания промокода (тикет)
* `created_by`              - staff login создателя промокода
* `expiration_msk_time`     - момент, до которого нужно активировать промокод
* `filter_info`             -
