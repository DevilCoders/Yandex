#### Tracker CLOUDSUPPORT Issues

Выгрузка тикетов очереди CLOUDSUPPORT из трекера.


```bash
# prod
yt link --proxy hahn --target-path //home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/issues --link-path //home/cloud-dwh/data/prod/raw/yt/startrek/yandex-team/queue/CLOUDSUPPORT/issues

# preprod
yt link --proxy hahn --target-path //home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/issues --link-path //home/cloud-dwh/data/preprod/raw/yt/startrek/yandex-team/queue/CLOUDSUPPORT/issues
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/yt/startrek/yandex-team/queue/CLOUDSUPPORT/issues) / [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/yt/startrek/yandex-team/queue/CLOUDSUPPORT/issues)

- `id`
- `hashKey`
- `version`
- `key`
- `aliases`
- `queue` - Очередь. Только CLOUDSUPPORT.
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
