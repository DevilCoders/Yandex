#### Exported support comments (PROD)

Выгрузка комментариев из АПИ поддержки.

```bash
# prod
yt link --proxy hahn --target-path //home/cloud/billing/exported-support-tables/comments_prod --link-path //home/cloud-dwh/data/prod/raw/billing/exported-support-tables/comments

# preprod
yt link --proxy hahn --target-path //home/cloud/billing/exported-support-tables/comments_prod --link-path //home/cloud-dwh/data/preprod/raw/billing/exported-support-tables/comments_prod
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/billing/exported-support-tables/comments)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/billing/exported-support-tables/comments_prod)

- `attachments`
- `created_at`
- `created_by`
- `direction` - incoming/outcoming.
- `export_ts`
- `iam_user_id`
- `id`
- `state`
- `text`
- `ticket_id`
- `updated_at`
- `visible`
