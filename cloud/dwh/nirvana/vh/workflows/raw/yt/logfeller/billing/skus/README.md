#### Skus

SKUs (продукты)

Так как уже данные лежат в yt, то просто настроена ссылка:

```bash
# preprod
yt link --proxy hahn --target-path //home/logfeller/logs/yc-pre-billing-export-sku/1h --link-path //home/cloud-dwh/data/preprod/raw/logfeller/billing/skus

# prod
yt link --proxy hahn --target-path  //home/logfeller/logs/yc-billing-export-sku/1h --link-path //home/cloud-dwh/data/prod/raw/logfeller/billing/skus
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/logfeller/billing/skus)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/logfeller/billing/skus)
