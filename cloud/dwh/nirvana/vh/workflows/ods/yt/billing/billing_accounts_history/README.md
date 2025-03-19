#### billing accounts history:

Вычитывает последний (актуальный) снапшот таблицы  `billing_accounts_history`

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/billing_accounts_history)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/billing_accounts_history)
/ [PROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/ods/billing/billing_accounts_history)
/ [PREPROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/ods/billing/billing_accounts_history)

* `auto_grant_policies`
* `autopay_failures`
* `balance_client_id`
* `balance_contract_id`
* `balance_person_id`
* `balance`
* `is_suspended_by_antifraud` - аккаунт заблокирован в следствии фрода (логика вычисления - [тут](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts/resources/utils/billing_accounts.sql))
* `billing_threshold`
* `block_reason`
* `country_code`
* `created_at`
* `currency`
* `disabled_at`
* `fraud_detected_by`
* `has_floating_threshold`
* `has_partner_access`
* `billing_account_id` - идентификатор аккаунта
* `idempotency_checks`
* `is_fraud`
* `is_isv`
* `is_referral`
* `is_referrer`
* `is_var`
* `is_verified`
* `master_account_id`
* `is_subaccount`
* `name`
* `owner_passport_uid`
* `paid_at`
* `payment_cycle_type`
* `payment_method_id`
* `payment_type`
* `person_id`
* `person_type`
* `registration_ip`
* `registration_iam_uid`
* `state`
* `type`
* `unblock_reason`
* `updated_at`
* `usage_status`
