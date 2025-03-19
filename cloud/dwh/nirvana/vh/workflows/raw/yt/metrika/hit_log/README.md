#### Hits log

Данные о хитах (событиях на счетчике).

Так как уже данные лежат в yt, то просто настроена ссылка:

```bash
# preprod
yt link --proxy hahn --target-path //logs/hit-log/1d --link-path //home/cloud-dwh/data/preprod/raw/metrika/hit_log

# prod
yt link --proxy hahn --target-path //logs/hit-log/1d --link-path //home/cloud-dwh/data/prod/raw/metrika/hit_log
```

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/metrika/hit_log)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/metrika/hit_log)

##### Схема

Прод и препрод не отличаются. Схема находится [тут](https://wiki.yandex-team.ru/jandexmetrika/data/metrikatables/hits/)

