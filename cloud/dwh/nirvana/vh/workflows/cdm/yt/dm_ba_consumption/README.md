#### Data Mart Billing Accounts Consumption

Витрина данных о потреблении услуг Яндекс.Облака обогащенная информацией о биллинг аккаунтах и sku.

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/dm_ba_consumption)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/dm_ba_consumption)
/ [PROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/cdm/dm_ba_consumption)
/ [PREPROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/cdm/dm_ba_consumption)


## Billing Account

| Domain          | Column                                              | Description |
| --------------- | --------------------------------------------------- | ----------- |
| Billing Account | `billing_account_id`                                | Идентификатор [платежного аккаунта](../../../ods/yt/billing/billing_accounts) |
| Billing Account | `billing_account_name`                              | Пользовательское название платежного аккаунта |
| Billing Account | `billing_account_type`                              | Тип биллинг акканута на `date` |
| Billing Account | `billing_account_type_current`                      | Тип биллинг акканута (сейчас) |
| Billing Account | `billing_account_is_suspended_by_antifraud`         |  - Флаг `is_suspended_by_antifraud` у биллинг аккаунта на дату `date` |
| Billing Account | `billing_account_is_suspended_by_antifraud_current` |  - Флаг `is_suspended_by_antifraud` у биллинг аккаунта (сейчас) |
| Billing Account | `billing_master_account_id`                         | идентификатор VAR'a (чей сабаккаунт) |
| Billing Account | `billing_master_account_name`                       | Имя биллинг аккаунта VAR'a (чей сабаккаунт) |
| Billing Account | `billing_account_month_cohort`                      | Месяц создания биллинг аккаунта в формате `%Y-%m` (например, `2021-12`) в таймзоне `Europe/Moscow` |
| Billing Account | `billing_account_currency`                          | Валюта платежного аккаунта (RUB, KZT, USD) |
| Billing Account | `billing_account_person_type`                       | Тип плательщика в Яндекс.Баланс на `date` |
| Billing Account | `billing_account_person_type_current`               | Тип плательщика в Яндекс.Баланс (сейчас) |
| Billing Account | `billing_account_state`                             | Состояние биллинг аккаунта на дату `date` |
| Billing Account | `billing_account_state_current`                     | Состояние биллинг аккаунта (сейчас) |
| Billing Account | `billing_account_usage_status`                      | Cтатус потребления биллинг аккаунта на дату `date` |
| Billing Account | `billing_account_usage_status_current`              | Cтатус потребления биллинг аккаунта (сейчас) |
| Billing Account | `billing_account_is_fraud`                          | - Флаг `is_fraud` у биллинг аккаунта на дату `date` |
| Billing Account | `billing_account_is_fraud_current`                  | - Флаг `is_fraud` у биллинг аккаунта (сейчас) |
| Billing Account | `billing_account_payment_type`                      | - Тип платежного средства (карта, перевод)  на дату `date` |
| Billing Account | `billing_account_payment_type_current`              | - Тип платежного средства - карта, перевод (сейчас) |
| Billing Account | `billing_account_is_isv`                            | -  Является ли биллинг аккаунт ISV на дату `date` |
| Billing Account | `billing_account_is_isv_current`                    | -  Является ли биллинг аккаунт ISV (сейчас) |
| Billing Account | `billing_account_is_var`                            | -  Является ли биллинг аккаунт VAR на дату `date` |
| Billing Account | `billing_account_is_var_current`                    | -  Является ли биллинг аккаунт VAR  (сейчас) |
| Billing Account | `billing_account_is_subaccount`                     | -  Является ли биллинг аккаунт сабаккаунтом |
| Billing Account | `billing_account_country_code`                      | - Код страны |

#### Типы аккаунта

| billing_account_type | Значение |
| -- | -- |
| *self-served* | аккаунт действует по договор-оферте (односторонний)
| *invoiced* | aккаунт действует по коммерческому договору (двусторонний) с ООО Яндекс Облако

---------

#### Типы состояний аккаунта

| `billing_account_state` | Значение | Ресурсы активны ? (clouds, folders, trackers) |
| --- | --- | -- |
| *new* | Идет процесс создания аккаунта (служебный статус)| нет| *payment_not_confirmed* | Зарегистрирован, юр.лицо, ожидает подтверждения документов после регистрации в backoffice | да
| *payment_required* | Есть платное потребление, выставлен акт, долг за неуплату в срок. Ждем оплаты.| да
| *suspended* |  Заблокирован (неуплата, фрод). Еще пытаемся списать деньги. Может вернуться в *active*.| нет
| *inactive* | Инактивирован. По просьбе пользователя - 90%, 10% - duty ad-hoc. Нет возможности восстановить аккаунт | нет
| *active* | Нормальное состояние, есть деньги, ресурсы активны | да
| *deleted* | Удален из базы, нигде не показывается, актуально для сабаккаунтов (не подтвердили партнерство). Нет возможности вернуть аккаунт.| нет
| *pending* | Служебный статус. Аккаунт зарегистрирован, но не активирован. |  нет

---------

#### Статус потребления аккаунта

| `billing_account_usage_status` | Значение |
| --- | --- |
| *trial* | Триальный биллинг аккаунт. Не пытаемся списать деньги, после траты грантов - либо переход в платное, либо блокировка. Между моментом блокировки и окончанием грантов все платное потребление уходит в `trial_credit`. Поле `total` всегда 0.
| *paid* | Платный биллинг аккаунт. Списываем деньги, генерируем акты, выставляем счета. Поле может `total` > 0
| *service* |  Служебный статус для системных Облаков (selfhost). Наши виртуалки, облака, etc. Все потребление уходит в total=0 (без платного). Выдается специальный тип кредита - service_credit.
| *disabled* | Статус для инактивированных (*state=inactive*) по той или иной причине аккаунтов. Все платное потребление уходит в credit, total = 0. Выдается специальный тип кредита - disabled_credit.

---------

**SKU:**

* `sku_id` - id SKU
* `sku_service_group` - группа сервиса SKU
* `sku_service_name` - сервис SKU
* `sku_service_name_eng` - название сервиса SKU (EN)
* `sku_subservice_name` - подсервис SKU
* `sku_name` - имя SKU
* `sku_name_eng` - публичное название SKU (EN)
* `sku_name_rus` - публичное название SKU (RU)
* `sku_pricing_unit` - единица измерения SKU
* `sku_lazy` - lazy SKU

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
