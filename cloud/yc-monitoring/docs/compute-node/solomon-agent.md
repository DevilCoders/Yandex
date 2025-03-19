[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=namespace%3Dycloud%26service%3Dsolomon-agent%26host%3Dyc_common_compute*)

## solomon-agent
Проверяет, что сервис `solomon-agent` работает (на Compute Nodes).
Загорается `WARN`, если есть ошибки в логах (например, из плагинов).
Загорается `CRIT`, если агент остановлен.

## Подробности
Solomon-agent на compute-nodes собирает метрики сервисов (compute-node, vrouter-agent, ...) и пользовательских инстансов. Периодически за этими метриками приходит Solomon fetcher. Если есть проблемы с ##solomon-agent##, то метрики могут пропадать с графиков.

Сервис также собирает **метрики, отображаемые пользователям в разделе "Мониторинг" веб-консоли.**
![](https://jing.yandex-team.ru/files/simonov-d/Screenshot%20from%202021-07-07%2018-17-31.png)

Сервис на ноде коммунальный, в нем запущены плагины разных команд (не только Compute).

## Диагностика
- Если горит `WARN`. Скорее всего, проблема с каким-то плагином (например, `yc_solomon_plugins.VRouterDnsLatencyMetrics: EXEC_STATUS failed 1 time(s)`). Нужно уточнить, что в логах по проблемному плагину, идти с этим к дежурному команды-владельца плагина. Владельца можно найти по имени плагина + blame в [репозитории solomon-agent-plugins](https://bb.yandex-team.ru/plugins/servlet/search?q=project%3ACLOUD%20repo%3Asolomon-agent-plugins%20VRouterDnsLatencyMetrics).
```
(DEV) simonov-d@cnvm-vla3:~$ sudo journalctl -u solomon-agent --since -1min | grep "yc_solomon_plugins.VRouterDnsLatencyMetrics" -A5
Jul 07 13:28:00 cnvm-vla3.svc.hw-load.cloud-lab.yandex.net solomon-agent[2832]: 2021-07-07 13:28:00.005 ERROR {data_puller.cpp:156}: error while pulling yc_solomon_plugins.VRouterDnsLatencyMetrics: (NPython2::TRuntimeError) solomon/agent/modules/pull/python2/python2.cpp:142: cannot pull from module yc_solomon_plugins.VRouterDnsLatencyMetrics: Traceback (most recent call last):
Jul 07 13:28:00 cnvm-vla3.svc.hw-load.cloud-lab.yandex.net solomon-agent[2832]:   File "/usr/lib/python2.7/dist-packages/yc_solomon_plugins/opencontrail/vrouter.py", line 356, in pull
Jul 07 13:28:00 cnvm-vla3.svc.hw-load.cloud-lab.yandex.net solomon-agent[2832]:     xml = fetch_xml("http://127.0.0.1:8085/Snh_SandeshTraceRequest?x=DnsBind", VROUTER_INTROSPECTION_TIMEOUT)
Jul 07 13:28:00 cnvm-vla3.svc.hw-load.cloud-lab.yandex.net solomon-agent[2832]:   File "/usr/lib/python2.7/dist-packages/yc_solomon_plugins/common.py", line 289, in fetch_xml
Jul 07 13:28:00 cnvm-vla3.svc.hw-load.cloud-lab.yandex.net solomon-agent[2832]:     raise RuntimeError("Can't get introspection from {}: {}".format(url, e.reason))
Jul 07 13:28:00 cnvm-vla3.svc.hw-load.cloud-lab.yandex.net solomon-agent[2832]: RuntimeError: Can't get introspection from http://127.0.0.1:8085/Snh_SandeshTraceRequest?x=DnsBind: [Errno 111] Connection refused
--
Jul 07 13:28:30 cnvm-vla3.svc.hw-load.cloud-lab.yandex.net solomon-agent[2832]: 2021-07-07 13:28:30.004 ERROR {data_puller.cpp:156}: error while pulling yc_solomon_plugins.VRouterDnsLatencyMetrics: (NPython2::TRuntimeError) solomon/agent/modules/pull/python2/python2.cpp:142: cannot pull from module yc_solomon_plugins.VRouterDnsLatencyMetrics: Traceback (most recent call last):
Jul 07 13:28:30 cnvm-vla3.svc.hw-load.cloud-lab.yandex.net solomon-agent[2832]:   File "/usr/lib/python2.7/dist-packages/yc_solomon_plugins/opencontrail/vrouter.py", line 356, in pull
Jul 07 13:28:30 cnvm-vla3.svc.hw-load.cloud-lab.yandex.net solomon-agent[2832]:     xml = fetch_xml("http://127.0.0.1:8085/Snh_SandeshTraceRequest?x=DnsBind", VROUTER_INTROSPECTION_TIMEOUT)
Jul 07 13:28:30 cnvm-vla3.svc.hw-load.cloud-lab.yandex.net solomon-agent[2832]:   File "/usr/lib/python2.7/dist-packages/yc_solomon_plugins/common.py", line 289, in fetch_xml
Jul 07 13:28:30 cnvm-vla3.svc.hw-load.cloud-lab.yandex.net solomon-agent[2832]:     raise RuntimeError("Can't get introspection from {}: {}".format(url, e.reason))
Jul 07 13:28:30 cnvm-vla3.svc.hw-load.cloud-lab.yandex.net solomon-agent[2832]: RuntimeError: Can't get introspection from http://127.0.0.1:8085/Snh_SandeshTraceRequest?x=DnsBind: [Errno 111] Connection refused
```
- Если горит `CRIT`. Сервис скорее всего не работает. Если окружение PROD - **караул, смотрим его логи и пытаемся запустить**.

## Ссылки
- [Документация по solomon-agent](https://wiki.yandex-team.ru/solomon/agent/)
- [Внутренние метрики агента](https://solomon.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod_compute&service=solomon_agent&l.host=myt1-ct3-7&l.sensor=storage.*&l.storageShard=yandexcloud%2Fcompute_node&graph=auto&stack=false)
