#### Billing Accounts history

История изменений биллинг аккаунтов

Так как уже данные лежат в yt, то просто настроена ссылка:

```bash
# preprod
yt link --proxy hahn --target-path //home/logfeller/logs/yc-pre-billing-export-billing-accounts-history/1h --link-path //home/cloud-dwh/data/preprod/raw/logfeller/billing/billing_accounts_history

# prod
yt link --proxy hahn --target-path  //home/logfeller/logs/yc-billing-export-billing-accounts-history/1h --link-path //home/cloud-dwh/data/prod/raw/logfeller/billing/billing_accounts_history
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/logfeller/billing/billing_accounts_history)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/logfeller/billing/billing_accounts_history)
