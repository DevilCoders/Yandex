#### Data Mart YC Consumption
#dm #billing #crm #consumption

Витрина данных о потреблении услуг Яндекс.Облака для дашборда [YC Consumption](https://datalens.yandex-team.ru/5etm2nlkxccuz-yc-consumption)

### Расположение данных

| Контур  | Расположение данных                                                                                                     |
|---------|-------------------------------------------------------------------------------------------------------------------------|
| PROD    | [dm_yc_consumption](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/dm_yc_consumption)    |
| PREPROD | [dm_yc_consumption](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/dm_yc_consumption) |



## Billing Account

| Поле                                              | Описание                                                                                           |
|---------------------------------------------------|----------------------------------------------------------------------------------------------------|
| billing_account_id                                | Идентификатор [платежного аккаунта](../../../ods/yt/billing/billing_accounts)                      |
| billing_account_name                              | Пользовательское название платежного аккаунта                                                      |
| billing_account_type                              | Тип биллинг акканута на `date`                                                                     |
| billing_account_type_current                      | Тип биллинг акканута (сейчас)                                                                      |
| billing_account_is_suspended_by_antifraud         | Флаг `is_suspended_by_antifraud` у биллинг аккаунта на дату `date`                                 |
| billing_account_is_suspended_by_antifraud_current | Флаг `is_suspended_by_antifraud` у биллинг аккаунта (сейчас)                                       |
| billing_master_account_id                         | идентификатор VAR'a (чей сабаккаунт)                                                               |
| billing_master_account_name                       | Имя биллинг аккаунта VAR'a (чей сабаккаунт)                                                        |
| billing_account_month_cohort                      | Месяц создания биллинг аккаунта в формате `%Y-%m` (например, `2021-12`) в таймзоне `Europe/Moscow` |
| billing_account_currency                          | Валюта платежного аккаунта (RUB, KZT, USD)                                                         |
| billing_account_person_type                       | Тип плательщика в Яндекс.Баланс на `date`                                                          |
| billing_account_person_type_current               | Тип плательщика в Яндекс.Баланс (сейчас)                                                           |
| billing_account_state                             | Состояние биллинг аккаунта на дату `date`                                                          |
| billing_account_state_current                     | Состояние биллинг аккаунта (сейчас)                                                                |
| billing_account_usage_status                      | Cтатус потребления биллинг аккаунта на дату `date`                                                 |
| billing_account_usage_status_current              | Cтатус потребления биллинг аккаунта (сейчас)                                                       |
| billing_account_is_fraud                          | Флаг `is_fraud` у биллинг аккаунта на дату `date`                                                  |
| billing_account_is_fraud_current                  | Флаг `is_fraud` у биллинг аккаунта (сейчас)                                                        |
| billing_account_payment_type                      | Тип платежного средства (карта, перевод)  на дату `date`                                           |
| billing_account_payment_type_current              | Тип платежного средства - карта, перевод (сейчас)                                                  |
| billing_account_is_isv                            | Является ли биллинг аккаунт ISV на дату `date`                                                     |
| billing_account_is_isv_current                    | Является ли биллинг аккаунт ISV (сейчас)                                                           |
| billing_account_is_var                            | Является ли биллинг аккаунт VAR на дату `date`                                                     |
| billing_account_is_var_current                    | Является ли биллинг аккаунт VAR  (сейчас)                                                          |
| billing_account_is_subaccount                     | Является ли биллинг аккаунт сабаккаунтом                                                           |
| billing_account_country_code                      | Код страны
| billing_record_max_end_time                       | Техническое поле. Используется для мониторинга.

#### Типы аккаунта

| billing_account_type | Описание                                                                       |
|----------------------|--------------------------------------------------------------------------------|
| self-served          | аккаунт действует по договор-оферте (односторонний)                            |
| invoiced             | aккаунт действует по коммерческому договору (двусторонний) с ООО Яндекс Облако |


#### Типы состояний аккаунта

| billing_account_state | Описание                                                                                                                         | Ресурсы активны ? (clouds, folders, trackers) |
|-----------------------|----------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------|
| new                   | Идет процесс создания аккаунта (служебный статус)                                                                                | нет                                           |
| payment_not_confirmed | Зарегистрирован, юр.лицо, ожидает подтверждения документов после регистрации в backoffice                                        | да                                            |
| payment_required      | Есть платное потребление, выставлен акт, долг за неуплату в срок. Ждем оплаты.                                                   | да                                            |
| suspended             | Заблокирован (неуплата, фрод). Еще пытаемся списать деньги. Может вернуться в *active*.                                          | нет                                           |
| inactive              | Инактивирован. По просьбе пользователя - 90%, 10% - duty ad-hoc. Нет возможности восстановить аккаунт                            | нет                                           |
| active                | Нормальное состояние, есть деньги, ресурсы активны                                                                               | да                                            |
| deleted               | Удален из базы, нигде не показывается, актуально для сабаккаунтов (не подтвердили партнерство). Нет возможности вернуть аккаунт. | нет                                           |
| pending               | Служебный статус. Аккаунт зарегистрирован, но не активирован.                                                                    | нет                                           |


#### Статус потребления аккаунта

| billing_account_usage_status | Описание                                                                                                                                                                                                                                     |
|------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| trial                        | Триальный биллинг аккаунт. Не пытаемся списать деньги, после траты грантов - либо переход в платное, либо блокировка. Между моментом блокировки и окончанием грантов все платное потребление уходит в `trial_credit`. Поле `total` всегда 0. |
| paid                         | Платный биллинг аккаунт. Списываем деньги, генерируем акты, выставляем счета. Поле может `total` > 0                                                                                                                                         |
| service                      | Служебный статус для системных Облаков (selfhost). Наши виртуалки, облака, etc. Все потребление уходит в total=0 (без платного). Выдается специальный тип кредита - service_credit.                                                          |
| disabled                     | Статус для инактивированных (*state=inactive*) по той или иной причине аккаунтов. Все платное потребление уходит в credit, total = 0. Выдается специальный тип кредита - disabled_credit.                                                    |


## CRM:

| Поле                        | Описание                                                                                                                                                                                                             |
|-----------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| crm_account_id              | id аккаунта в CRM                                                                                                                                                                                                    |
| crm_account_name            | название аккаунта из CRM                                                                                                                                                                                             |
| crm_account_dimensions      | "аналитические разрезы" (или просто "разрезы"). [Подробнее](https://wiki.yandex-team.ru/cloud/bizdev/automation/dimensions_v2/)                                                                                      |
| crm_account_tags            | идентификатор, который используется для категоризации, описания, поиска записей или задания внутренней  структуры. [Подробнее](https://wiki.yandex-team.ru/cloud/bizdev/automation/crm/crm-user-documentation/tags/) |                                                                                                                                 |
| crm_account_owner           | CRM Account Owner на дату `date`                                                                                                                                                                                     |
| crm_account_owner_current   | текущий CRM Account Owner                                                                                                                                                                                            |
| crm_architect               | CRM Architect на дату `date`                                                                                                                                                                                         |
| crm_architect_current       | текущий CRM Architect                                                                                                                                                                                                |
| crm_bus_dev                 | CRM Business Development Manager на дату `date`                                                                                                                                                                      |
| crm_bus_dev_current         | текущий CRM Business Development Manager                                                                                                                                                                             |
| crm_partner_manager         | CRM Partner Manager на дату `date`                                                                                                                                                                                   |
| crm_partner_manager_current | текущий CRM Partner Manager                                                                                                                                                                                          |
| crm_sales                   | CRM Sales на дату `date`                                                                                                                                                                                             |
| crm_sales_current           | текущий CRM Sales                                                                                                                                                                                                    |
| crm_segment                 | CRM Segment на дату `date`                                                                                                                                                                                           |
| crm_segment_current         | текущий CRM Segment                                                                                                                                                                                                  |
| crm_tam                     | CRM Technical Account Manager на дату `date`                                                                                                                                                                         |
| crm_tam_current             | текущий CRM Technical Account Manager                                                                                                                                                                                |
| account_display_name        | имя CRM аккаунта или Фамилия Имя физического лица.                                                                                                                                                                   |

## SKU

| Поле                 | Описание                    |
|----------------------|-----------------------------|
| поле                 | описание                    |
| sku_id               | id SKU                      |
| sku_service_group    | группа сервиса SKU          |
| sku_service_name     | сервис SKU                  |
| sku_service_name_eng | название сервиса SKU (EN)   |
| sku_subservice_name  | подсервис SKU               |
| sku_name             | имя SKU                     |
| sku_name_eng         | публичное название SKU (EN) |
| sku_name_rus         | публичное название SKU (RU) |
| sku_pricing_unit     | единица измерения SKU       |
| sku_lazy             | lazy SKU                    |

## Billing Records

| Поле                            | Описание                                                                                                     |
|---------------------------------|--------------------------------------------------------------------------------------------------------------|
| billing_record_origin_service   | сервис который инициировал потребление (пока поддерживается только 'mk8s', для остальных значение undefined) |
| billing_record_msk_date         | дата потребления в iso-формате `%Y-%m-%d` (например, `2021-12-31`) в таймзоне `Europe/Moscow`                |
| billing_record_msk_month_cohort | месяц потребления в формате `%Y-%m` (например, `2021-12`) в таймзоне `Europe/Moscow`                         |
| billing_record_msk_quarter      | квартал потребления (например, `2021-Q4`) в таймзоне `Europe/Moscow`                                         |
| billing_record_msk_half_year    | полугодие потребления (например, `2021-H2`) в таймзоне `Europe/Moscow`                                       |
| billing_record_msk_year         | год потребления (например, `2021`) в таймзоне `Europe/Moscow`                                                |

Cost:

| Поле                    | Описание                                                        |
|-------------------------|-----------------------------------------------------------------|
| billing_record_cost     | стоимость потребленного SKU в валюте `billing_account_currency` |
| billing_record_cost_rub | стоимость потребленного SKU в рублях                            |

## Pricing Quantity:
| Поле                            | Описание                     |
|---------------------------------|------------------------------|
| billing_record_pricing_quantity | количество потребленного SKU |


## Credit:

| Поле                                           | Описание                                                                                                             |
|------------------------------------------------|----------------------------------------------------------------------------------------------------------------------|
| billing_record_credit                          | общая скидка (сумма) в валюте `billing_account_currency` с НДС без VAR reward                                        |
| billing_record_credit_rub                      | общая скидка в рублях с НДС без VAR reward                                                                           |
| billing_record_credit_rub_vat                  | общая скидка в рублях без НДС без VAR reward                                                                         |
| billing_record_credit_monetary_grant           | скидка в валюте `billing_account_currency` с НДС от денежных грантов                                                 |
| billing_record_credit_monetary_grant_rub       | скидка в рублях с НДС от денежных грантов                                                                            |
| billing_record_credit_monetary_grant_rub_vat   | скидка в рублях от денежных грантов без НДС                                                                          |
| billing_record_credit_service                  | скидка в валюте `billing_account_currency` с НДС для сервисных платежных аккаунтов (usage_status = `service`)        |
| billing_record_credit_service_rub              | скидка в рублях с НДС для сервисных платежных аккаунтов (usage_status = `service`)                                   |
| billing_record_credit_service_rub_vat          | скидка в рублях без НДС для сервисных платежных аккаунтов (usage_status = `service`)                                 |
| billing_record_credit_trial                    | скидка в валюте `billing_account_currency` с НДС для триальных платежных аккаунтов потративших грант, но еще не paid |
| billing_record_credit_trial_rub                | скидка в рублях с НДС для триальных платежных аккаунтов потративших грант, но еще не paid                            |
| billing_record_credit_trial_rub_vat            | скидка в рублях без НДС для триальных платежных аккаунтов потративших грант, но еще не paid                          |
| billing_record_credit_cud                      | скидка от committed usage discount в валюте `billing_account_currency` с НДС                                         |
| billing_record_credit_cud_rub                  | скидка от committed usage discount в рублях с НДС                                                                    |
| billing_record_credit_cud_rub_vat              | скидка от committed usage discount в рублях без НДС                                                                  |
| billing_record_credit_volume_incentive         | скидка от volume discount в валюте `billing_account_currency`с НДС                                                   |
| billing_record_credit_volume_incentive_rub     | скидка от volume discount в рублях с НДС                                                                             |
| billing_record_credit_volume_incentive_rub_vat | скидка от volume discount в рублях без НДС                                                                           |
| billing_record_credit_disabled                 | скидка для инактивированных платежных аккаунтов (usage_status=disabled) в валюте `billing_account_currency` с НДС    |
| billing_record_credit_disabled_rub             | скидка для инактивированных платежных аккаунтов (usage_status=disabled) в рублях с НДС                               |
| billing_record_credit_disabled_rub_vat         | скидка для инактивированных платежных аккаунтов (usage_status=disabled) в рублях без НДС                             |

## Total && Expense && VAR Rewards:

| Поле                                        | Описание                                                                                                                                                                                               |
|---------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| billing_record_expense                      | итого к оплате с учетом скидок в валюте `billing_account_currency` (cost + credit), платное потребление без reward с НДС                                                                               |
| billing_record_expense_rub_vat              | итого к оплате рублях с учетом скидок (cost + credit), платное потребление без reward без НДС                                                                                                          |
| billing_record_expense_rub                  | итого к оплате в рублях с учетом скидок (cost + credit), платное потребление без reward с НДС                                                                                                          |
| billing_record_total                        | итого к оплате с учетом скидок в валюте `billing_account_currency` (cost + credit - reward), платное потребление с НДС                                                                                 |
| billing_record_total_rub                    | итого к оплате с учетом скидок в рублях (cost + credit - reward), платное потребление с НДС                                                                                                            |
| billing_record_total_rub_vat                | итого к оплате с учетом скидок в рублях (cost + credit - reward), платное потребление без НДС                                                                                                          |
| billing_record_total_redistribution         | итого к оплате с учетом скидок в валюте `billing_account_currency` (cost + credit - reward), платное потребление с НДС. Перераспределена сумма с резервовов на основной продукт (актуально для кубера) |
| billing_record_total_redistribution_rub     | итого к оплате с учетом скидок в рублях (cost + credit - reward), платное потребление с НДС. Перераспределена сумма с резервовов на основной продукт (актуально для кубера)                            |
| billing_record_total_redistribution_rub_vat | итого к оплате с учетом скидок в рублях (cost + credit - reward), платное потребление без НДС. Перераспределена сумма с резервовов на основной продукт (актуально для кубера)                          |
| billing_record_var_reward                   | var-премия в валюте `billing_account_currency` с НДС                                                                                                                                                   |
| billing_record_var_reward_rub               | var-премия в рублях с НДС                                                                                                                                                                              |
| billing_record_var_reward_rub_vat           | var-премия в рублях без НДС                                                                                                                                                                            |

## Real Consumption (Платное потребление):

| Поле                                    | Описание                                                                                  |
|-----------------------------------------|-------------------------------------------------------------------------------------------|
| billing_record_real_consumption_rub     | итоговая стоимость с учетом var-премии и скидок в рублях (cost + credit - reward) с НДС   |
| billing_record_real_consumption_rub_vat | итоговая стоимость с учетом var-премии и скидок в рублях (cost + credit - reward) без НДС |

## Price list consumption
Alpha dot't use it:

| Поле                                  | Описание                                      |
|---------------------------------------|-----------------------------------------------|
| price_list_consumption__alpha         | Потребление в текущих ценах в валюте          |
| price_list_consumption_rub__alpha     | Потребление в текущих ценах в рублях с НДС    |
| price_list_consumption_rub_vat__alpha | Потребление в текущих ценах в рублях без НДС  |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
