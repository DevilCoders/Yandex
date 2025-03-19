#### Data Mart Consumption with Marketing Attribution

Витрина данных о размеченном маркетингом потреблении услуг Яндекс.Облака для дашборда [YC Marketing Dash](https://datalens.yandex-team.ru/pfkwd12zt3msk-yc-marketing-dash)

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/marketing/dm_consumption_with_marketing_attribution)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/marketing/dm_consumption_with_marketing_attribution)



| Domain          | Column                                              | Description |
| --------------- | --------------------------------------------------- | ----------- |
| Billing Account | `billing_account_id`                                | Идентификатор [платежного аккаунта](../../../ods/yt/billing/billing_accounts) |
| Cloud           | `cloud_id`                                          | Идентификатор [облака](../../../ods/yt/billing/service_instance_bindings) |
| SKU             | `sku_id`                                            | Идентификатор [продукта](../../../ods/yt/billing/skus) |


**Billing Account:**
* `billing_account_month_cohort` - месяц создания платёжного аккаунта
* `billing_account_is_suspended_by_antifraud_current` - был ли платёжный аккаунт хотя бы раз заблокирован антифродом к текущему моменту
* `billing_account_name` - пользовательское название платежного аккаунта
* `person_type` - тип плательщика в Яндекс.Баланс на дату `billing_record_msk_date`
* `crm_segment` - CRM Segment на дату `billing_record_msk_date`

**SKU:**
* `sku_service_name` - сервис SKU
* `sku_subservice_name` - подсервис SKU
* `sku_name` - имя SKU
* `sku_service_group` - группа сервиса SKU

**Billing Records:**
* `billing_record_msk_date` - дата потребления в iso-формате `%Y-%m-%d` (например, `2021-12-31`) в таймзоне `Europe/Moscow`

**Consumption (Потребление):**
* `billing_record_total_rub_vat_by_event_first_weight_model` - (cost + credit - reward), платное потребление в рублях без НДС с применением модели `first`
* `billing_record_total_rub_vat_by_event_last_weight_model` - (cost + credit - reward), платное потребление в рублях без НДС с применением модели `last`
* `billing_record_total_rub_vat_by_event_uniform_weight_model` - (cost + credit - reward), платное потребление в рублях без НДС с применением модели `uniform`
* `billing_record_total_rub_vat_by_event_u_shape_weight_model` - (cost + credit - reward), платное потребление в рублях без НДС с применением модели `u_shape`
* `billing_record_total_rub_vat_by_event_exp_7d_half_life_time_decay_weight_model` - (cost + credit - reward), платное потребление в рублях без НДС с применением модели `exp_7d_half_life_time_decay`
* `billing_record_credit_monetary_grant_rub_vat_by_event_first_weight_model` - скидка в рублях от денежных грантов без НДС с применением модели `first`
* `billing_record_credit_monetary_grant_rub_vat_by_event_last_weight_model` - скидка в рублях от денежных грантов без НДС с применением модели `last`
* `billing_record_credit_monetary_grant_rub_vat_by_event_uniform_weight_model` - скидка в рублях от денежных грантов без НДС с применением модели `uniform`
* `billing_record_credit_monetary_grant_rub_vat_by_event_u_shape_weight_model` - скидка в рублях от денежных грантов без НДС с применением модели `u_shape`
* `billing_record_credit_monetary_grant_rub_vat_by_event_exp_7d_half_life_time_decay_weight_model` - скидка в рублях от денежных грантов без НДС с применением модели `exp_7d_half_life_time_decay`

**Marketing:**
* `channel_marketing_influenced` - маркер показывающий относится ли событие к Маркетинговым активностям
* `channel` - канал к которому относится событие
* `utm_source` - имя UTM метки, рекламная площадка (Вырезается из URL первого просмотра)
* `utm_medium` - тип рекламы UTM (Вырезается из URL первого просмотра)
* `utm_campaign` - Название рекламной компании. Маркетинговые мероприятия соответствуют следующему формату: `<источник трафика>_<страна>_<город>_<устройство>_<бюджет>_<Название продукта>_<название департамента>`
* `utm_term` - Ключевая фраза UTM (Вырезается из URL первого просмотра)
