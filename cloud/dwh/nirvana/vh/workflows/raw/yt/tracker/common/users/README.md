#### Startrek Users

TBD.

//home/startrek/tables/prod/yandex-team/common/users
```bash
# prod
yt link --proxy hahn --target-path //home/startrek/tables/prod/yandex-team/common/users --link-path //home/cloud-dwh/data/prod/raw/yt/startrek/yandex-team/common/users

# preprod
yt link --proxy hahn --target-path //home/startrek/tables/prod/yandex-team/common/users --link-path //home/cloud-dwh/data/preprod/raw/yt/startrek/yandex-team/common/users
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/yt/startrek/yandex-team/common/users) / [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/yt/startrek/yandex-team/common/users)

- `uid`
- `hashKey`
- `login`
- `firstName`
- `lastName`
- `dismissed`
- `external`
- `orgAdmin`
- `hasLicense`
- `robot`
- `webPushSubscriptionState`
- `langUi`
- `firstLoginDate`
- `orgId`
