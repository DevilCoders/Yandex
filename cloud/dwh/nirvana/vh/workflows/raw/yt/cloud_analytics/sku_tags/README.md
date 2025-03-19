#### Sku tags

Метки для SKU. Продовая таблица берется у Аналитиков. Для препрода эмулируется разметка SKU на основе продовой, подменяя идентификаторы

```bash
# prod
yt link --proxy hahn --target-path //home/cloud_analytics/export/billing/sku_tags/sku_tags --link-path //home/cloud-dwh/data/prod/raw/cloud_analytics/sku_tags
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/cloud_analytics/sku_tags)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/cloud_analytics/sku_tags)
