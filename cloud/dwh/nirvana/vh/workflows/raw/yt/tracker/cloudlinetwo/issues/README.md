#### Tracker CLOUDLINETWO Issues

Выгрузка тикетов очереди CLOUDLINETWO из трекера.


```bash
# prod
yt link --proxy hahn --target-path //home/startrek/tables/prod/yandex-team/queue/CLOUDLINETWO/issues --link-path //home/cloud-dwh/data/prod/raw/yt/startrek/yandex-team/queue/CLOUDLINETWO/issues

# preprod
yt link --proxy hahn --target-path //home/startrek/tables/prod/yandex-team/queue/CLOUDLINETWO/issues --link-path //home/cloud-dwh/data/preprod/raw/yt/startrek/yandex-team/queue/CLOUDLINETWO/issues
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/yt/startrek/yandex-team/queue/CLOUDLINETWO/issues) / [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/yt/startrek/yandex-team/queue/CLOUDLINETWO/issues)

- `id`
- `hashKey`
- `version`
- `key`
- `aliases`
- `queue` - Очередь. Только CLOUDLINETWO.
- `status`
- `resolution`
- `type`
- `summary`
- `description`
- `created`
- `updated`
- `start`
- `end`
- `dueDate`
- `resolved`
- `resolver`
- `author`
- `modifier`
- `assignee`
- `priority`
- `affectedVersions`
- `fixVersions`
- `components`
- `tags`
- `sprint`
- `customFields`
- `followers`
- `access`
- `unique`
- `followingGroups`
- `followingMaillists`
- `parent`
- `epic`
- `originalEstimation`
- `estimation`
- `spent`
- `storyPoints`
- `ranks`
- `ranking`
- `goals`
- `votedBy`
- `favoritedBy`
- `emailFrom`
- `emailTo`
- `emailCc`
- `emailCreatedBy`
- `sla`
- `orgId`
