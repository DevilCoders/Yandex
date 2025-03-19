## Meeseeks

[Активные тесты датаплейна сети](https://wiki.yandex-team.ru/cloud/devel/sdn/mrprober/) из виртуалок на каждой компьют-ноде.

Кластер виртуалок Meeseeks (по виртуалке на каждой компьют-ноде) выступает заменой для старых __e2e-tests-permnet-*__.

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


### network-ping-internal-target-...

Пингует //внутренние таргеты// — специальные машинки в разных зонах доступности внутри Облака. Загорается красным, если за последний час было >= 20 фейлов.


### http-request-interal-target-...

Аналогично предыдущему, но устанавливает HTTP-соединение.


### network-ping-internal-targets

Пингует внутренние таргеты (см. выше) и загорается красным, если с одного хоста недоступно сразу два таргета.


### http-request-internal-targets

Аналогично предыдущему, но устанваливает HTTP-соединение.


### ping-external-targets

Пингует список внешних таргетов. Загорается красным, если с одного хоста недоступно сразу два таргета.


### ping-....

Пингует список внешних таргетов. Загорается красным, если с заметного количества хостов недоступен какой-то внешний таргет.


### mr-prober-ping6-ns1.yandex.net-1min

Пингует `ns1.yandex.net` (`2a02:6b8::1`)


### mr-prober-dns-resolve-vm-self

Резолвит A-запись виртуалки, на которой запущен


### mr-prober-dns-resolve-external-hostnames

Резолвит github.com, yandex.ru, google.com и mail.ru


### mr-prober-dns-resolve-special-mdb-host

Резолвит запись `cloud-35256-use-for-dns-monitoring-only.mdb.yandexcloud.net`


### mr-prober-dns-resolve-vm-self-reverse

Резолвит PTR-запись виртуалки, на которой запущен


### mr-prober-dns-resolve-yandex-host

Резолвит запись %%storage-int.mds.yandex.net%%


### mr-prober-dns-resolve-yandex-ru

Резолвит yandex.ru через системный резолвер


### mr-prober-metadata-...

Проверяет работу сервиса метаданных
