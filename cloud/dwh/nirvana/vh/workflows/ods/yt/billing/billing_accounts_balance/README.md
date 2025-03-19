#### Billing Accounts Balance:

Отображает состояние баланса (поступления, долг, etc) **текущего** договора аккаунта.

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/billing_accounts_balance)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/billing_accounts_balance)

* `billing_account_id` - идентификатор [платежного аккаунта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts)
* `balance` - сумма баланса лицевого счета в валюте аккаунта
* `debt` - задолжность по актам за предыдущие периоды в валюте аккаунта
* `receipts` - сумма пополнений счета в валюте аккаунта
* `balance_modified_at` - дата модификации баланса (UTC)
* `debt_modified_at` - дата модификации задолжности (UTC)
* `receipts_modified_at` - дата модификации поступлений (UTC)
* `modified_at` - последнее обновление строки (UTC)
