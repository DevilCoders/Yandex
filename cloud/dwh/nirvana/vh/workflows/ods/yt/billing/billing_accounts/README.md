## Billing Accounts
#billing #billing_accounts

Информация о платежных аккаунтах пользователей Облака.

#### Ссылки
[Документация](https://cloud.yandex.com/en/docs/billing/concepts/billing-account)
[История изменений](../billing_accounts_history/)


### Расположение данных
| Контур            | Расположение данных   | Источники |
| ----------------- | --------------------- | --------- |
| PROD              | [ods/billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/billing_accounts)    | [raw/billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/meta/billing_accounts)     |
| PREPROD           | [ods/billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/billing_accounts) | [raw/billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/meta/billing_accounts)  |
| PROD_INTERNAL     | [ods/billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/ods/billing/billing_accounts) | [raw/billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/raw/ydb/billing/meta/billing_accounts)  |
| PREPROD_INTERNAL  | [ods/billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/ods/billing/billing_accounts) | [raw/billing_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/raw/ydb/billing/meta/billing_accounts)  |


### Структура
| Поле                          | Описание                                          |
| ----------------------------- | ------------------------------------------------- |
| `billing_account_id`          | Идентификатор аккаунта.                           |
| `auto_grant_policies`         |  |
| `autopay_failures`            |  |
| `balance_client_id`           |  |
| `balance_contract_id`         |  |
| `balance_person_id`           |  |
| `balance`                     |  |
| `is_suspended_by_antifraud`   | аккаунт заблокирован в следствии фрода. Логика вычисления флага - [тут](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts/resources/utils/billing_accounts.sql) |
| `billing_threshold`           |  |
| `block_reason`                |  |
| `country_code`                | Код страны, откуда клиент.                        |
| `created_at`                  | Дата и время создания платежного аккаунта, UTC.   |
| `currency`                    | Валюта аккаунта. [Подробнее](https://cloud.yandex.com/en/docs/billing/payment/currency) |
| `disabled_at`                 | Дата и время блокировки аккаунта, UTC. |
| `fraud_detected_by`           |  |
| `has_floating_threshold`      |  |
| `has_partner_access`          |  |
| `idempotency_checks`          |  |
| `is_fraud`                    | Внутренний биллинговый флаг. Рекомендуем использовать `is_suspended_by_antifraud`. |
| `is_isv`                      | участник программы [Cloud Boost](https://cloud.yandex.ru/cloud-boost), которая направлена на поддержку ISV(Independent Software Vendor). |
| `is_referral`                 |  |
| `is_referrer`                 |  |
| `is_var`                      | Является ли [партнером (Value Added Reseller)](https://cloud.yandex.ru/docs/partner/program/var). |
| `is_verified`                 |  |
| `master_account_id`           | ID родительского аккаунта. |
| `is_subaccount`               | Является ли аккаунт дочерним. В таком случае имеется и `master_account_id`. |
| `name`                        |  |
| `owner_passport_uid`          |  |
| `owner_iam_uid`               |  |
| `paid_at`                     |  |
| `payment_cycle_type`          |  |
| `payment_method_id`           |  |
| `payment_type`                |  |
| `person_type`                 | [Тип](https://cloud.yandex.ru/docs/billing/concepts/billing-account#ba-types) платежного аккаунта. |
| `registration_ip`             |  |
| `registration_iam_uid`        |  |
| `state`                       | Статус платежного аккаунта. [Подробнее](https://cloud.yandex.com/en/docs/billing/concepts/billing-account-statuses)) |
| `type`                        | _invoiced_, _self-served_ |
| `unblock_reason`              |  |
| `updated_at`                  | Дата и время обновления, UTC. |
| `usage_status`                | _trial_, _disabled_, _paid_, _service_ |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
