# Migrations for cloud analytics YT tables

## Examples
```
~/src/arcadia/cloud/analytics/migrations/export/marketo$ ya make .
~/src/arcadia/cloud/analytics/migrations/export/marketo$ ./migrate cloud.analytics.migrations.export.marketo //projects/marketo-testing/export --proxy hume
# Reset all migrations
~/src/arcadia/cloud/analytics/migrations/export/marketo$ ./migrate cloud.analytics.migrations.export.marketo //projects/marketo-testing/export zero --proxy hume
# Apply specific migration
~/src/arcadia/cloud/analytics/migrations/export/marketo$ ./migrate cloud.analytics.migrations.export.marketo //projects/marketo-testing/export 0001_add_total_amount_column --proxy hume
~/src/arcadia/cloud/analytics/migrations/export/marketo$ ./migrate cloud.analytics.migrations.export.marketo //home/cloud_analytics/export/marketo --proxy hahn
```
To update the dynamic table:
```
~/src/arcadia/cloud/analytics/migrations/leads$ ya make .
~/src/arcadia/cloud/analytics/migrations/leads$ ./migrate cloud.analytics.migrations.leads //home/cloud_analytics/leads 0008_added_ba_fields --proxy hahn
```

## Conventions

Schema version is tracked on prefix level, so by convention migrations should be placed in dirs relative to 
//home/cloud_analytics yt path
