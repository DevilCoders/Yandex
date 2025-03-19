#### Email click log

Информация об открытиях писем, кликах и отписках.

```bash
#create folder

# preprod
yt create map_node --proxy hahn //home/cloud-dwh/data/preprod/raw/sender
yt link --proxy hahn --target-path //logs/sendr-delivery-log/1d --link-path //home/cloud-dwh/data/preprod/raw/sender/email_click_log

# prod
yt create map_node --proxy hahn //home/cloud-dwh/data/prod/raw/sender
yt link --proxy hahn --target-path //logs/sendr-delivery-log/1d --link-path //home/cloud-dwh/data/prod/raw/sender/email_click_log
```

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/sender/email_click_log)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/sender/email_click_log)

##### Схема

Прод и препрод не отличаются. Схема находится [тут](https://wiki.yandex-team.ru/sender/logs/)

