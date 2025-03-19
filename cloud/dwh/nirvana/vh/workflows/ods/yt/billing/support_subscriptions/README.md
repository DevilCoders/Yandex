#### Billing Accounts Support Level

Отображает подписки платной поддержки аккаунтов и их уровень. Подписки может и не быть у аккаунта, что означает Basic (Free) уровень поддержки.

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/support_subscriptions)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/support_subscriptions)


* `billing_account_id` - идентификатор [платежного аккаунта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts)
* `subscription_id` - идентификатор подписки
* `priority_level_code` - код приоритета 1,2,3
* `priority_level_name` - имя приоритета (Standard, Business, Premium)
* `start_time_msk` - время начала действия (МСК)
* `end_time_msk` - время конца действия (МСК)
