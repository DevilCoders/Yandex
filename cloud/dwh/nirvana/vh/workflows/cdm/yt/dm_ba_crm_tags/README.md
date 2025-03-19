#### Data Mart BA CRM tags

Витрина, в которой содержится исторические данные о биллинг аккаунтах с CRM-атрибутами.

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/dm_ba_crm_tags)

* `date` - дата в iso-формате `%Y-%m-%d` (например, `2021-12-31`) в таймзоне `Europe/Moscow`
* `billing_account_id` - id биллинг аккаунта
* `billing_account_name` - пользовательское название биллинг аккаунта
* `billing_account_month_cohort` - месяц создания биллинг аккаунта в формате `%Y-%m` (например, `2021-12`) в таймзоне `Europe/Moscow`
* `currency` - валюта биллинг аккаунта
* `is_subaccount` - является ли биллинг аккаунт сабаккаунтам VAR
* `country_code` - код страны биллинг аккаунта
* `crm_account_id` - id аккаунта в CRM
* `crm_account_name` - название аккаунта из CRM
* `client_id` - id клиента.
    * `crm_acc_{crm_account_id}` - для биллинг аккаунтов, у которых есть аккаунт в CRM
    * `ba_{billing_account_id}` - для биллинг аккаунтов, у которых нет аккаунта в CRM
* `crm_account_dimensions` - "аналитические разрезы" (или просто "разрезы"). [Подробнее](https://wiki.yandex-team.ru/cloud/bizdev/automation/dimensions_v2/)
* `crm_account_tags` - идентификатор, который используется для категоризации, описания, поиска записей или задания внутренней
  структуры. [Подробнее](https://wiki.yandex-team.ru/cloud/bizdev/automation/crm/crm-user-documentation/tags/)
* `person_type` - тип плательщика на дату `date`
* `person_type_current` - текущий тип плательщика
* `usage_status` - тип потребления на дату `date`
* `usage_status_current` - текущий тип потребления
* `state` - статус биллинг аккаунта на дату `date`
* `state_current` - текущий статус биллинг аккаунта
* `is_fraud` - is_fraud биллинг аккаунта на дату `date`
* `is_fraud_current` - текущий is_fraud биллинг аккаунта
* `payment_type` - тип оплаты биллинг аккаунта на дату `date`
* `payment_type_current` - текущий тип оплаты биллинг аккаунта
* `is_isv` - является ли биллинг аккаунт ISV на дату `date`
* `is_isv_current` - является ли биллинг аккаунт ISV
* `is_var` - является ли биллинг аккаунт VAR на дату `date`
* `is_var_current` - является ли биллинг аккаунт VAR
* `account_owner` - CRM Account Owner на дату `date`
* `account_owner_current` - текущий CRM Account Owner
* `architect` - CRM Architect на дату `date`
* `architect_current` - текущий CRM Architect
* `bus_dev` - CRM Business Development Manager на дату `date`
* `bus_dev_current` - текущий CRM Business Development Manager
* `partner_manager` - CRM Partner Manager на дату `date`
* `partner_manager_current` - текущий CRM Partner Manager
* `sales` - CRM Sales на дату `date`
* `sales_current` - текущий CRM Sales
* `segment` - CRM Segment на дату `date`
* `segment_current` - текущий CRM Segment
* `tam` - CRM Technical Account Manager на дату `date`
* `tam_current` - текущий CRM Technical Account Manager
