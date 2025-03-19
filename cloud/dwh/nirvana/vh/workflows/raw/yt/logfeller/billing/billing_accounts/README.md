#### Billing Accounts

Биллинг аккаунты

Так как уже данные лежат в yt, то просто настроена ссылка:

```bash
# preprod
yt link --proxy hahn --target-path //home/logfeller/logs/yc-pre-billing-export-billing-accounts/1h --link-path //home/cloud-dwh/data/preprod/raw/logfeller/billing/billing_accounts

# prod
yt link --proxy hahn --target-path  //home/logfeller/logs/yc-billing-export-billing-accounts/1h --link-path //home/cloud-dwh/data/prod/raw/logfeller/billing/billing_accounts
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/logfeller/billing/billing_accounts)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/logfeller/billing/billing_accounts)
