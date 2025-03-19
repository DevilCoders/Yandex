#### Exported support comments (PROD)

Выгрузка комментариев из АПИ поддержки.

```bash
# prod
yt link --proxy hahn --target-path //home/cloud/billing/exported-support-tables/tickets_prod --link-path //home/cloud-dwh/data/prod/raw/billing/exported-support-tables/tickets

# preprod
yt link --proxy hahn --target-path //home/cloud/billing/exported-support-tables/tickets_prod --link-path //home/cloud-dwh/data/preprod/raw/billing/exported-support-tables/tickets_prod
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/billing/exported-support-tables/tickets)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/billing/exported-support-tables/tickets_prod)

- `access_type`
- `attachments`
- `cloud_id`
- `comments_metadata`
- `created_at`
- `description`
- `export_ts`
- `folder_id`
- `iam_user_id`
- `id`
- `last_comment`
- `meta`
- `resolved_at`
- `st_id`
- `st_key` - ID тикета в Startrek.
- `st_version`
- `state`
- `summary`
- `synced_at`
- `type`
- `updated_at`
- `updated_at_desc`