# vpc-solomon-quotas-cgw

**Что проверяет**: Прогноз квоты в solomon, на основе текущих данных, на 7 и 14 дней. Желтый если на 14 дней не хватает, красный если на 7 или уже превысили.
**Подробности:** не звонящий.
Что делать:
1. Перейти по [ссылке](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts/solomon_vpc_quotas_preprod) в juggler алерте
2. Посмотреть на график usage, нет ли резкого выброса новых метрик, возможно выехал релиз/флуд от клиентов, тогда разбираемся внутри, в details есть shardId с которым проблема
3. Проверить наличие Sensors TTL на [shardId](https://solomon.yandex-team.ru/admin/projects/yandexcloud/shards/yandexcloud_cloud_prod_compute_oct_vrouter) или [service](https://solomon.yandex-team.ru/admin/projects/yandexcloud/services/yandexcloud_oct_vrouter) из details алерта. Если не был выставлен, то поправить для сервиса в [vpc-solomon](https://bb.yandex-team.ru/projects/CLOUD/repos/vpc-solomon) и прокатить изменения(см. README)
4. Если выставлен ttl, данные валидные, то необходимо запросить расширение квоты через [форму](https://forms.yandex-team.ru/surveys/28780/), нужно указать:
```Идентификатор проекта: yandexcloud
Идентификатор шарда: shardId из details алерта
Тип квоты: Количество метрик в шарде
Требуемое значение: х2 от текущего, либо больше, если мы знаем объем валидных новых метрик
Причина запроса: ссылка на текущий алерт с объяснением проблемы
```
