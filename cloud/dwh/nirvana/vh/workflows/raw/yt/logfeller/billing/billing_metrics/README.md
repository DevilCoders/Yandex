#### billing_metrics

Так как уже данные лежат в yt, то просто настроена ссылка:

```bash
# preprod
yt link --proxy hahn --target-path //home/logfeller/logs/yc-billing-metrics --link-path //home/cloud-dwh/data/preprod/raw/logfeller/billing/billing_metrics
# prod
yt link --proxy hahn --target-path //home/logfeller/logs/yc-billing-metrics --link-path //home/cloud-dwh/data/prod/raw/logfeller/billing/billing_metrics
```

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/logfeller/billing/billing_metrics) / [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/logfeller/billing/billing_metrics)

