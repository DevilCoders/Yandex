#### Data Mart Folder Consumption

Витрина данных о потреблении услуг Яндекс.Облака для дашборда [ML Services](https://datalens.yandex-team.ru/kmlmcvc5ckxkg-ml-services?tab=8gv)

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/dm_folder_consumption)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/dm_folder_consumption)



| Domain          | Column                                              | Description |
| --------------- | --------------------------------------------------- | ----------- |
| Billing Account | `billing_account_id`                                | Идентификатор [платежного аккаунта](../../../ods/yt/billing/billing_accounts) |
| Cloud           | `cloud_id`                                          | Идентификатор [облака](../../../ods/yt/billing/service_instance_bindings) |
| Folder          | `folder_id`                                         | Идентификатор [раздела](../../../ods/yt/iam/folders) |
| SKU             | `sku_id`                                            | Идентификатор [продукта](../../../ods/yt/billing/skus) |

**IDs**
* `folder_id` - id фолдера
* `cloud_id` - id облака

**Billing Account:**
* `account_display_name_hash` - хеш от имя CRM аккаунта или Фамилия Имя физического лица.

**SKU:**

- `sku_id` - id SKU
- `sku_service_name` - сервис SKU
- `sku_service_name_eng` - название сервиса SKU (EN)
- `sku_subservice_name` - подсервис SKU
- `sku_name` - имя SKU
- `sku_name_eng` - публичное название SKU (EN)
- `sku_name_rus` - публичное название SKU (RU)
- `sku_pricing_unit` - единица измерения SKU

**Folder:**
- `folder_id` - id раздела
- `folder_name` - имя раздела

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

### PII


[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/PII/dm_folder_consumption)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/PII/dm_folder_consumption)

#### Схема:
* `billing_account_id` - идентификатор платежного аккаунта
* `billing_record_msk_date` - дата потребления в iso-формате `%Y-%m-%d` (например, `2021-12-31`) в таймзоне `Europe/Moscow`
* `account_display_name` - имя CRM аккаунта или Фамилия Имя физического лица.
