#### transactions

Часовые таблицы транзакций из Логфеллера.

Так как уже данные лежат в yt, то просто настроена ссылка:

PROD:
```bash
yt link --proxy hahn --target-path //home/logfeller/logs/yc-billing-export-transactions/1h --link-path //home/cloud-dwh/data/prod/raw/logfeller/billing/transactions
```

PREPROD:
```bash
yt link --proxy hahn --target-path //home/logfeller/logs/yc-pre-billing-export-transactions/1h --link-path //home/cloud-dwh/data/preprod/raw/logfeller/billing/transactions
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/logfeller/billing/transactions)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/logfeller/billing/transactions)
