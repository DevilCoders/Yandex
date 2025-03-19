#### Tracker CLOUDLINETWO Issue Events

Выгрузка истории изменений тикетов очереди CLOUDLINETWO из трекера.


```bash
# prod
yt link --proxy hahn --target-path //home/startrek/tables/prod/yandex-team/queue/CLOUDLINETWO/issue_events --link-path //home/cloud-dwh/data/prod/raw/yt/startrek/yandex-team/queue/CLOUDLINETWO/issue_events

# preprod
yt link --proxy hahn --target-path //home/startrek/tables/prod/yandex-team/queue/CLOUDLINETWO/issue_events --link-path //home/cloud-dwh/data/preprod/raw/yt/startrek/yandex-team/queue/CLOUDLINETWO/issue_events
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/yt/startrek/yandex-team/queue/CLOUDLINETWO/issue_events) / [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/yt/startrek/yandex-team/queue/CLOUDLINETWO/issue_events)

- `id`
- `issue` - ID [тикета](../issues).
- `hashKey`
- `date`
- `changes`
- `comments`
- `links`
- `author`
- `attachments`
- `orgId`
- `localFieldChanges`