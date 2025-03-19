## World Connectivity

[Активные тесты датаплейна сети](https://wiki.yandex-team.ru/cloud/devel/sdn/mrprober/)
для проверки внешней связности.

На каждом стенде развёрнуто два кластера: `world-via-fip` и `world-via-egress-nat`,
которые проверяют связность с внешним миром через FIP-ы и Egress NAT соответственно.

В общем случае загоревшийся алерт выглядит так

```
Alert: success(e279850ba345c39cbe51ce772e1e77c6cff98799)
Changed at: 2021-09-09T05:03:27.345Z
Status: ALARM
cluster: meeseeks

host: vla04-2ct28-28a.meeseeks.prober.cloud-preprod.yandex.net

logs: https://console.cloud.yandex.ru/folders/yc.vpc.mr-prober/storage/bucket/mr-prober-logs?key=preprod%2Fprobers%2Fmeeseeks%2Fvla04-2ct28-28a.meeseeks.prober.cloud-preprod.yandex.net%2Fnetwork-ping-8.8.8.8%2F

fails graph: https://solomon.yandex-team.ru/?project=cloud_mr_prober&service=metrics&l.metric=fail&cluster=preprod&l.prober_slug=network-ping&l.host=vla04-2ct28-28a.meeseeks.prober.cloud-preprod.yandex.net&l.cluster_slug=meeseeks&graph=auto&b=1h&e=&downsamplingFill=previous

prober: network-ping-8.8.8.8

service: mr-prober-network-ping-8.8.8.8

stand: preprod

Url: https://solomon.yandex-team.ru/admin/projects/cloud_mr_prober/alerts/fail/subAlerts/e279850ba345c39cbe51ce772e1e77c6cff98799
```

В алерте важно:
- `host: vla04-2ct28-28a.meeseeks.prober.cloud-preprod.yandex.net` — можно зайти с помощью `pssh` на проблемную виртуалку, подёргать команды, посмотреть логи агента `docker logs --tail 100 -f agent.service` или `journalctl -u agent`, а также логи конкретного пробера (теста): `/var/log/mr_prober/...`
- `fails graph: ...` — ссылка на график в соломоне, где видны все упавшие проберы. Можно поставить `*` вместо имени пробера, чтобы посмотреть, падали ли другие проберы, или вместо хоста, чтобы посмотреть, была ли проблема массовой. Доступны агрегатные хосты `Vla`, `Sas` и `Myt`, чтобы оценить проблему во всей зоне доступности.
- `logs: https://console.cloud.yandex.ru/folders/yc.vpc.mr-prober/storage/bucket/mr-prober-logs?key=preprod%2Fprobers%2Fmeeseeks%2Fvla04-2ct28-28a.meeseeks.prober.cloud-preprod.yandex.net%2Fnetwork-ping-8.8.8.8%2F` — можно зайти в S3, прямо в веб-интерфейсе найти логи неуспешного запуска и посмотреть их (кнопка «Скачать» за тремя точками в правом верхнем углу)
- `Url: https://solomon.yandex-team.ru/admin/projects/cloud_mr_prober/alerts/fail/subAlerts/e279850ba345c39cbe51ce772e1e77c6cff98799` — ссылка на соломон, где можно посмотреть историю срабатывания алерта, график времени выполнения (поменять `metric=fail`на `metric=duration_milliseconds`) и так далее


### ping-external-targets

Пингует список внешних таргетов. Загорается красным, если с одного хоста недоступно сразу два таргета.


### ping-....

Пингует список внешних таргетов. Загорается красным, если с заметного количества хостов недоступен какой-то внешний таргет.


### smtp-available-...

Проверяет доступность `mx.yandex.ru:25` и `smtp.yandex.ru:25`. Загорается красным, если за последний час таргет был недоступен 20 или больше раз.


### smtp-unavailable-...

Проверяет **недоступность** `smtp.gmail.com:25`. Загорается красным, если удалось подключиться.
