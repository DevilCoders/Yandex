[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=nsb+client)

## Описание
Горит, если процент ошибок в вызовах клиента NBS превысил установленный порог.

## Подробности
При работе тасок, требующих выполнение действий на стороне NBS, возникли ошибки. Важно, что часто ошибки пишутся с severity `warn`.

## Диагностика
- Убедится на [графике](https://grafana.yandex-team.ru/d/VdSkkchZk/cloud-compute-tasks?orgId=1&viewPanel=40), что проблема имеет место.
- По [графику](https://monitoring.yandex-team.ru/projects/yandexcloud/explorer/queries?range=1d&q.0.s=%7Bproject%3D%22yandexcloud%22%2C%20service%3D%22compute_tasks%22%2C%20cluster%3D%22cloud_prod_head%22%2C%20sensor%3D%22nbs_client_requests%22%2C%20error_type%21%3D%22OK%22%2C%20host%3D%22%2A%22%7D&normz=off&colors=auto&type=auto&interpolation=linear&dsp_method=auto&dsp_aggr=default&dsp_fill=default) определить хост и временной интервал, на котором наблюдались ошибки.
- На хосте из предыдущего пункта окрыть логи сервиса yc-compute-tasks и получить сообщение об ошибке: `journalctl -u yc-compute-tasks.service -p4 -S -15m`
