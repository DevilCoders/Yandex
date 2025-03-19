#### Data Mart Base Consumption

Витрина данных о потреблении услуг Яндекс.Облака без дополнительной информации

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/dm_base_consumption)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/dm_base_consumption)
/ [PROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/cdm/dm_base_consumption)
/ [PREPROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/cdm/dm_base_consumption)


## Billing Account

| Domain          | Column                                              | Description |
| --------------- | --------------------------------------------------- | ----------- |
| Billing Account | `billing_account_id`                                | Идентификатор [платежного аккаунта](../../../ods/yt/billing/billing_accounts) |
| Billing Account | `billing_account_currency`                          | Валюта платежного аккаунта (RUB, KZT, USD) |

**IDs**
* `resource_id` - id ресурса
* `folder_id` - id фолдера
* `cloud_id` - id облака


**SKU:**

* `sku_id` - id SKU

**Billing Records:**

* `billing_record_origin_service` - сервис который инициировал потребление (пока поддерживается только 'mk8s', для остальных значение undefined)
* `billing_record_msk_date` - дата потребления в iso-формате `%Y-%m-%d` (например, `2021-12-31`) в таймзоне `Europe/Moscow`
* `billing_record_msk_month_cohort` - месяц потребления в формате `%Y-%m` (например, `2021-12`) в таймзоне `Europe/Moscow`
* `billing_record_msk_quarter` - квартал потребления (например, `2021-Q4`) в таймзоне `Europe/Moscow`
* `billing_record_msk_half_year` - полугодие потребления (например, `2021-H2`) в таймзоне `Europe/Moscow`
* `billing_record_msk_year` - год потребления (например, `2021`) в таймзоне `Europe/Moscow`

Cost:

* `billing_record_cost` - стоимость потребленного SKU в валюте `billing_account_currency`
* `billing_record_cost_rub` - стоимость потребленного SKU в рублях

Pricing Quantity:
* `billing_record_pricing_quantity` -  количество потребленного SKU


Credit:


* `billing_record_credit` - общая скидка (сумма) в валюте `billing_account_currency` с НДС без VAR reward
* `billing_record_credit_rub` - общая скидка в рублях с НДС без VAR reward
* `billing_record_credit_rub_vat` - общая скидка в рублях без НДС без VAR reward

* `billing_record_credit_monetary_grant` - скидка в валюте `billing_account_currency` с НДС от денежных грантов
* `billing_record_credit_monetary_grant_rub` - скидка в рублях с НДС от денежных грантов
* `billing_record_credit_monetary_grant_rub_vat` - скидка в рублях от денежных грантов без НДС

* `billing_record_credit_service` - скидка в валюте `billing_account_currency` с НДС для сервисных платежных аккаунтов (usage_status = `service`)
* `billing_record_credit_service_rub` - скидка в рублях с НДС для сервисных платежных аккаунтов (usage_status = `service`)
* `billing_record_credit_service_rub_vat` - скидка в рублях без НДС для сервисных платежных аккаунтов (usage_status = `service`)

* `billing_record_credit_trial` - скидка в валюте `billing_account_currency` с НДС для триальных платежных аккаунтов потративших грант, но еще не paid
* `billing_record_credit_trial_rub` - скидка в рублях с НДС для триальных платежных аккаунтов потративших грант, но еще не paid
* `billing_record_credit_trial_rub_vat` - скидка в рублях без НДС для триальных платежных аккаунтов потративших грант, но еще не paid

* `billing_record_credit_cud` - скидка от committed usage discount в валюте `billing_account_currency` с НДС
* `billing_record_credit_cud_rub` - скидка от committed usage discount в рублях с НДС
* `billing_record_credit_cud_rub_vat`  - скидка от committed usage discount в рублях без НДС

* `billing_record_credit_volume_incentive` - скидка от volume discount в валюте `billing_account_currency`с НДС
* `billing_record_credit_volume_incentive_rub` - скидка от volume discount в рублях с НДС
* `billing_record_credit_volume_incentive_rub_vat` - скидка от volume discount в рублях без НДС

* `billing_record_credit_disabled` - скидка для инактивированных платежных аккаунтов (usage_status=disabled) в валюте `billing_account_currency` с НДС
* `billing_record_credit_disabled_rub` - скидка для инактивированных платежных аккаунтов (usage_status=disabled) в рублях с НДС
* `billing_record_credit_disabled_rub_vat` - скидка для инактивированных платежных аккаунтов (usage_status=disabled) в рублях без НДС

Total && Expense && VAR Rewards:

* `billing_record_expense`- итого к оплате с учетом скидок в валюте `billing_account_currency` (cost + credit), платное потребление без reward с НДС
* `billing_record_expense_rub_vat`- итого к оплате рублях с учетом скидок (cost + credit), платное потребление без reward без НДС
* `billing_record_expense_rub`- итого к оплате в рублях с учетом скидок (cost + credit), платное потребление без reward с НДС

* `billing_record_total`- итого к оплате с учетом скидок в валюте `billing_account_currency` (cost + credit - reward), платное потребление с НДС
* `billing_record_total_rub`- итого к оплате с учетом скидок в рублях (cost + credit - reward), платное потребление с НДС
* `billing_record_total_rub_vat`- итого к оплате с учетом скидок в рублях (cost + credit - reward), платное потребление без НДС

* `billing_record_var_reward`- var-премия в валюте `billing_account_currency` с НДС
* `billing_record_var_reward_rub`- var-премия в рублях с НДС
* `billing_record_var_reward_rub_vat`- var-премия в рублях без НДС

Real Consumption (Платное потребление):

* `billing_record_real_consumption_rub`- итоговая стоимость с учетом var-премии и скидок в рублях (cost + credit - reward) с НДС
* `billing_record_real_consumption_rub_vat`- итоговая стоимость с учетом var-премии и скидок в рублях (cost + credit - reward) без НДС
