#### Startrek cloud support issue comments

TBD.


```bash
# prod
yt link --proxy hahn --target-path //home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/comments --link-path //home/cloud-dwh/data/prod/raw/yt/startrek/yandex-team/queue/CLOUDSUPPORT/comments

# preprod
yt link --proxy hahn --target-path //home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/comments --link-path //home/cloud-dwh/data/preprod/raw/yt/startrek/yandex-team/queue/CLOUDSUPPORT/comments
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/yt/startrek/yandex-team/queue/CLOUDSUPPORT/comments) / [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/yt/startrek/yandex-team/queue/CLOUDSUPPORT/comments)

- `id`
- `issue`
- `hashKey`
- `shortId`
- `version`
- `text`
- `author`
- `modifier`
- `summonees`
- `maillistSummonees`
- `created`
- `updated`
- `deleted`
- `email`
- `emailsInfo`
- `reactions`
- `orgId`
