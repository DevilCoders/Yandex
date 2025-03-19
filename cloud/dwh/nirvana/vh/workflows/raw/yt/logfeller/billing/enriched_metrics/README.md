#### Enriched metrics

Обогащенные метрики потребления сервисов биллинга.

Так как уже данные лежат в yt, то просто настроена ссылка:

```bash
# preprod
yt link --proxy hahn --target-path //home/logfeller/logs/yc-pre-billing-rt-enricher-var-output --link-path //home/cloud-dwh/data/preprod/raw/logfeller/billing/enriched_metrics

# prod
yt link --proxy hahn --target-path //home/logfeller/logs/yc-billing-rt-enricher-var-output --link-path //home/cloud-dwh/data/prod/raw/logfeller/billing/enriched_metrics

# prod_internal
yt link --proxy hahn --target-path //home/logfeller/logs/yc-yandex-billing-rt-enricher-base-output --link-path //home/cloud-dwh/data/prod_internal/raw/logfeller/billing/enriched_metrics

# preprod_internal
yt link --proxy hahn --target-path //home/logfeller/logs/yc-billing-internal-prestable-rt-enricher-base-output --link-path //home/cloud-dwh/data/preprod_internal/raw/logfeller/billing/enriched_metrics
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/logfeller/billing/enriched_metrics)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/logfeller/billing/enriched_metrics)
/ [PROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/raw/logfeller/billing/enriched_metrics)
/ [PREPROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/raw/logfeller/billing/enriched_metrics)

* `billing_account_id` - id биллинг аккаунта
* `cloud_id` - id облака
* `folder_id` - id фолдера
* `resource_id` - id ресурса
* `master_account_id`- id биллинг аккаунта, который является VAR-ом для `billing_account_id`

* `usage_time` - время потребления в формате unix timestamp. Вычисляется как `usage.finish - 1`
* `start_time`- время потребления в формате unix timestamp с округлением до начала часа
* `end_time` - время потребления в формате unix timestamp с округлением до конца часа

* `currency` - валюта
* `currency_multiplier` - коэффициент перевода из рублей в `currency`. Может быть равен `1` даже для не рублевых метрик в случае наличия sku override с флагом local_currency

* `schema` - схема метрики
* `service_id`- id сервиса, к которому принадлежит sku
* `sku_id` - id SKU
* `sku_name`- имя SKU
* `pricing_version_id` - id использованной цены из `sku.pricing_versions[*].id`
* `rate_id` - id rate для пороговых sku
* `pricing_unit`- тип единицы потребления для подсчета стоимости
* `unit_price` - цена за единицу `pricing_unit`
* `sku_overridden`- флаг, который указывает, был ли применен sku override
* `tiered_pricing_quantity`- размер потребления в единицах `pricing_unit`, которые были учтены в текущем пороге sku (для пороговых sku)
* `pricing_quantity` - размер потребления в единицах `pricing_unit`
* `cost` - стоимость в валюте `currency`
* `credit`- общая скидка
* `expense` - итого к оплате с учетом скидок в валюте `currency`
* `monetary_grant_credit`- скидка от Monetary Grant
* `cud_credit` - скидка от CUD (Committed Use Discount, CVoS)
* `cud_compensated_pricing_quantity`- размер потребления в единицах `pricing_unit`, которые были покрыты с помощью CUD
* `volume_incentive_credit` - скидка от Volume Incentive
* `trial_credit` - скидка заблокированного триального пользователя
* `disabled_credit` - скидка инактивированного аккаунта
* `service_credit`- скидка для сервисных аккаунтов
* `credit_charges`- yson-поле со списком примененных скидок

* `rewarded_expense` - стоимость потребления, от которого посчитано поле `volume_reward`
* `volume_reward` - var-премия в валюте `currency`
* `reward` - var-премия в валюте `currency`
* `volume_reward_info`- yson-поле со списком примененных var-скидок
* `var_incentive_credit`- _deprecated_. Скидка var-партнера

* `publisher_account_id` - id паблишера продукта
* `publisher_balance_client_id`- id клиента баланса, который принадлежит паблишеру
* `publisher_currency`- валюта паблишера
* `revenue` - премия паблишеру в валюте `publisher_currency`

* `labels` - системные и пользовательские метки (label)
* `labels_json` - json представление поля `labels`
* `labels_hash`- хеш поля `labels`

* `tags` - yson-поле c служебными теги сервиса
* `usage` - yson-поле c информацией о потреблении от сервиса
* `id` - id метрики
* `source_wt` - время создания метрики сервисом в формате unix timestamp
* `message_write_ts` - служебное поле Logbroker. Время записи сообщения
* `version`- служебное поле биллинга
* `resharding_key`- служебное поле биллинга
* `source_id`- служебное поле сервиса

* `_rest` - поля, которые не попали в парсер logfeller
* `_stbx` - служебное поле logfeller в формате `topic:partition@@offset@@sourceid@@ctime@@exporttime@@logtype@@seqno@@wtime`
* `source_uri` - служебное поле logfeller
* `iso_eventtime`- служебное поле Logfeller
* `_logfeller_index_bucket`- служебное поле Logfeller
* `_logfeller_timestamp`- служебное поле Logfeller
