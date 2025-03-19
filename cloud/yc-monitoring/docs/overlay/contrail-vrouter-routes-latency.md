[Алерт contrail-vrouter-routes-latency в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-vrouter-routes-latency)

## Что проверяет

Время прошедшее между отправкой маршрута из vrouter в contrail-control (XmppRouteExport) и получением этого же маршрута обратно из contrail-control в vrouter (XmppRouteImport)

## Если загорелось

- Убедиться что latency рассчитана верно (по логу /var/log/contrail/sandesh/sandesh-route.log)

- Проверить загруженность CPU на oct голове с которой долго шел маршрут [ссылка](https://monitoring.yandex-team.ru/projects/yandexcloud/explorer/queries?q.0.s=%7Bproject%3D%22yandexcloud%22%2C%20cluster%3D%22cloud_prod_oct%22%2C%20service%3D%22oct_head_cgroup_metrics%22%2C%20metric%3D%22cpuacct.usage%22%2C%20unit%3D%22contrail-control%22%2C%20host%3D%22oct-vla1%22%7D&range=1d&normz=off&colors=auto&type=line&interpolation=linear&dsp_method=auto&dsp_aggr=default&dsp_fill=default&vis_labels=off&vis_aggr=avg)

- Проверить нет ли роста waitq на oct голове с которой долго шел маршрут [ссылка](https://monitoring.yandex-team.ru/projects/yandexcloud/explorer/queries?q.0.s=%7Bproject%3D%22yandexcloud%22%2C%20cluster%3D%22cloud_prod_oct%22%2C%20service%3D%22oct_control%22%2C%20name%3D%22tasks.entries.waitq%22%2C%20host%3D%22oct-vla1%22%2C%20group_name%3D%22bgp%3A%3A%2A%22%7D&range=1d&normz=off&colors=auto&type=area&interpolation=linear&dsp_method=auto&dsp_aggr=default&dsp_fill=default&vis_labels=off&vis_aggr=avg)
