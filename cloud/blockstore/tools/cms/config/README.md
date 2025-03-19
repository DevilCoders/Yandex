# Утилита для изменения/просмотра настроек blockstore-server

## Обновление настроек

По умолчанию обновляются настройки на всех локациях продакшена:
```bash
blockstore-config update --config-name ActorSystemConfig --proto nbs/nbs-sys.txt
```


С помощью опции `dc` можно явно указать локации:
```bash
blockstore-config update --dc sas vla --config-name ActorSystemConfig --proto nbs/nbs-sys.txt
```


Если добавить параметр `preprod`, то обновляться будет препрод:
```bash
blockstore-config update --preprod --config-name ActorSystemConfig --proto nbs/nbs-sys.txt
```


Для обновления настроек конкретной машины (или нескольких) нужно указать `host`:
```bash
blockstore-config update --config-name ActorSystemConfig --proto nbs/nbs-sys.txt --host sas09-s1-7.cloud.yandex.net sas09-s1-10.cloud.yandex.net
```


## Удаление настроек

```bash
blockstore-config remove --config-name ActorSystemConfig LogConfig
```

## Просмотр настроек

По умолчанию будут выведены настройки всех локаций продакшена:
```bash
blockstore-config view
```


С помощью опции `dc` можно явно указать локации:
```bash
blockstore-config view --dc sas vla
```


Если добавить параметр `preprod`, то будут показаны настройки перпрода:
```bash
blockstore-config view --preprod
```


Можно посмотреть наскройки конкретной машины (или нескольких):
```bash
blockstore-config view --host sas09-s1-7.cloud.yandex.net
```

Чтобы посмотреть, какие настройки в каких местах записаны (файлы или cms), можно указать параметр `split`:
```bash
blockstore-config view --host sas09-s1-7.cloud.yandex.net --split
```


Настройки выводятся в json-формате, поэтому удобно использовать утилиту `jq`:
```bash
$ blockstore-config view --dc sas | jq -r '.sas.ActorSystemConfig.Executor[] | "\(.Name): \(.Threads)"'
System: 3
User: 3
Batch: 1
IO: 1
IC: 3
```

Так можно пропатчить конфиг:
```bash
$ blockstore-config -vv view --cluster preprod --dc vla | jq '.vla.StorageServiceConfig + { MaxCompactionDelay: 5000 }' > new-config.json
$ blockstore-config -vv update --cluster preprod --dc vla --config-name StorageServiceConfig --proto new-config.json
```
