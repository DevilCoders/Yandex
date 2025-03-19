#### Antifraud falsepositive Billing Accounts:

Перечень фолсов антифрода с датой блокировки на основе [истории изменения биллинг аккаунта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts_history). Фолсом считается заблокированный антифродом аккаунт, впоследствии разблокированный любым способом вне зависимости от последующих после разблокировки блокировок и текущего статуса

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/antifraud/dm_ba_af_falsepositive)

- billing_account_id -
  Идентификатор [платежного аккаунта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts/README.md)
- blocked_at - Время когда произошла блокировка
