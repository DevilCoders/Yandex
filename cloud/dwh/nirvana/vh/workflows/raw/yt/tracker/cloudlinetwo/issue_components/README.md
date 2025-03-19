#### Tracker CLOUDLINETWO issue components

Выгрузка компонентов для тикетов из очереди CLOUDLINETWO.


```bash
# prod
yt link --proxy hahn --target-path //home/startrek/tables/prod/yandex-team/queue/CLOUDLINETWO/components --link-path //home/cloud-dwh/data/prod/raw/yt/startrek/yandex-team/queue/CLOUDLINETWO/components

# preprod
yt link --proxy hahn --target-path //home/startrek/tables/prod/yandex-team/queue/CLOUDLINETWO/components --link-path //home/cloud-dwh/data/preprod/raw/yt/startrek/yandex-team/queue/CLOUDLINETWO/components
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/yt/startrek/yandex-team/queue/CLOUDLINETWO/components) / [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/yt/startrek/yandex-team/queue/CLOUDLINETWO/components)


- `id`,
- `hashKey`,
- `shortId`,
- `version`,
- `queue` - очередь, только CLOUDLINETWO.
- `name`,
- `description`,
- `lead`,
- `assignAuto`,
- `permissions`,
- `followers`,
- `followingGroups`,
- `followingMaillists`,
- `deleted`,
- `created`,
- `updated`,
- `orgId`