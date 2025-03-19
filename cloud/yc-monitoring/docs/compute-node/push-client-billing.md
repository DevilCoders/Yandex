[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=namespace%3Dycloud%26service%3Dpush-client-billing)

## push-client-billing
Загорается, если есть проблемы с доставкой биллинговых данных через push-client в Logbroker.

## Подробности
Сервис push-client на ноде коммунальный, в нем множество конфигов:
```
(TESTING) root@myt1-ct5-3:~# ls -la /etc/yandex/statbox-push-client/conf.d/
total 32
drwxr-xr-x 2 statbox statbox 4096 Jul  8 09:09 .
drwxr-xr-x 3 root    root    4096 Jul  8 06:59 ..
-rw-r--r-- 1 statbox statbox  718 Mar  3 14:59 push-client-billing.yaml
-rw-r--r-- 1 statbox statbox  559 Mar  3 14:59 push-client-cloud_logs.yaml
-rw-r--r-- 1 statbox statbox  548 Mar  3 14:59 push-client-compute_cp_vpc.yaml
-rw-r--r-- 1 statbox statbox  506 Mar  3 14:59 push-client-e2e-sli.yaml
-rw-r--r-- 1 statbox statbox  563 Mar  3 14:59 push-client-sdn_antifraud.yaml
-rw-r--r-- 1 statbox statbox  543 Mar  3 14:59 push-client-solomon-agent-dump.yaml
```

Для команды Compute важен конфиг `/etc/yandex/statbox-push-client/conf.d/push-client-billing.yaml` - через него отправляются данные о биллинге инстансов (из `/var/lib/yc/compute-node/accounting/accounting.log`). Мониторинг проверяет работу push-client именно с этим конфигом.

## Диагностика
1. Если массово загорелось на всех хостах PROD & PRE-PROD - вероятно, ведутся работы в Logbroker VLA (проверить - см. ссылки ниже на infra или чат). **Биллинговые данные отправляются только в один ДЦ (для снижения риска перебилла)**. В этом случае мы просто ждем окончания работ.
2. Запущен ли сервис? `systemctl status statbox-push-client`
3. Есть ли ошибки в логах? `tail /var/log/statbox/*.log`
4. Смотрим статус отправки данных по конкретному конфигу:
   - `push-client -c /etc/yandex/statbox-push-client/conf.d/push-client-billing.yaml --status`
   - Как понять вывод push-client?
     - _Все работает хорошо, в файл постоянно пишут и он отправляется в LB:_
       - в `last_send_time` текущее время, в `last_commit_time` текущее время. В поле `lag` выводится размер последнего куска неотправленных данных и он (размер) обычно достаточно мал.
     - _Все работает хорошо, но в файл давно не писали:_
        - в `last_send_time` текущее время, в `last_commit_time` время последней отправки в LB (обычно совпадает с временем записи в файл), в поле `lag` 0.
     - _Push-client не может начать отправку в LB:_
       - в поле `last_send_time` будет время последнего успешного соединения с LB (время старта push-client, если ни разу с LB не удалось установить связь)
     - _Push-client успешно соединяется с LB, но отправка данных не работает:_
       - в `last_send_time` текущее время, в `last_commit_time` время последней отправки в LB, в поле `lag` ненулевое значение

Пример 1: есть проблема "Push-client успешно соединяется с LB, но отправка данных не работает"
```
(TESTING) root@myt1-ct5-3:~# push-client -c /etc/yandex/statbox-push-client/conf.d/push-client-e2e-sli.yaml --status 2>&1  | egrep "(':|last_send_time|last_commit_time|commit delay|lag)"
[Jul 08 09:10:20.287341] myt1-ct5-3.cloud.yandex.net [3770.4312]:  '/var/log/e2e-sli/e2e_sli.log':
[Jul 08 09:10:20.287424] myt1-ct5-3.cloud.yandex.net [3770.4312]:    commit delay: 759528
[Jul 08 09:10:20.287430] myt1-ct5-3.cloud.yandex.net [3770.4312]:    lag: 10485651
[Jul 08 09:10:20.287450] myt1-ct5-3.cloud.yandex.net [3770.4312]:    last_commit_time: 1614784089 (03.03.2021-15.08.09)
[Jul 08 09:10:20.287457] myt1-ct5-3.cloud.yandex.net [3770.4312]:    last_send_time: 1625735412 (08.07.2021-09.10.12)
[Jul 08 09:10:20.287472] myt1-ct5-3.cloud.yandex.net [3770.4312]:  '/var/log/e2e-sli/e2e_sli.log':
[Jul 08 09:10:20.287533] myt1-ct5-3.cloud.yandex.net [3770.4312]:    commit delay: 651864
[Jul 08 09:10:20.287538] myt1-ct5-3.cloud.yandex.net [3770.4312]:    lag: 6893091
[Jul 08 09:10:20.287607] myt1-ct5-3.cloud.yandex.net [3770.4312]:    last_commit_time: 1623770618 (15.06.2021-15.23.38)
[Jul 08 09:10:20.287619] myt1-ct5-3.cloud.yandex.net [3770.4312]:    last_send_time: 1625735412 (08.07.2021-09.10.12)
[Jul 08 09:10:20.287633] myt1-ct5-3.cloud.yandex.net [3770.4312]:  '/var/log/e2e-sli/e2e_sli.log':
[Jul 08 09:10:20.287694] myt1-ct5-3.cloud.yandex.net [3770.4312]:    commit delay: 759528
[Jul 08 09:10:20.287699] myt1-ct5-3.cloud.yandex.net [3770.4312]:    lag: 10485665
[Jul 08 09:10:20.287716] myt1-ct5-3.cloud.yandex.net [3770.4312]:    last_commit_time: 1620781875 (12.05.2021-01.11.15)
[Jul 08 09:10:20.287725] myt1-ct5-3.cloud.yandex.net [3770.4312]:    last_send_time: 1625735412 (08.07.2021-09.10.12)
[Jul 08 09:10:20.287738] myt1-ct5-3.cloud.yandex.net [3770.4312]:  '/var/log/e2e-sli/e2e_sli.log':
[Jul 08 09:10:20.287804] myt1-ct5-3.cloud.yandex.net [3770.4312]:    commit delay: 759528
[Jul 08 09:10:20.287809] myt1-ct5-3.cloud.yandex.net [3770.4312]:    lag: 10485670
[Jul 08 09:10:20.287826] myt1-ct5-3.cloud.yandex.net [3770.4312]:    last_commit_time: 1617793028 (07.04.2021-10.57.08)
[Jul 08 09:10:20.287835] myt1-ct5-3.cloud.yandex.net [3770.4312]:    last_send_time: 1625735412 (08.07.2021-09.10.12)

(TESTING) root@myt1-ct5-3:~# tail /var/log/statbox/e2e-sli.log -n2
[Jul 08 09:11:50.471309] myt1-ct5-3.cloud.yandex.net [3770.4412]: INFO:	pqlib sid=4G4xbL1LxQrz7Wvp-TP41g, session=: Subproducer start response: { error { code: BAD_REQUEST description: "topic \'rt3.myt--yc-df-pre--e2e-sli\' describe error, Status# StatusPathDoesNotExist, reason: Path not found, Marker# PQ1" } }
[Jul 08 09:11:50.471321] myt1-ct5-3.cloud.yandex.net [3770.4412]: WARN:	pqlib sid=4G4xbL1LxQrz7Wvp-TP41g, session=: Subproducer start error: { error { code: BAD_REQUEST description: "topic \'rt3.myt--yc-df-pre--e2e-sli\' describe error, Status# StatusPathDoesNotExist, reason: Path not found, Marker# PQ1" } }
```
Пример 2. Все хорошо:
```
(TESTING) root@myt1-ct5-3:~# push-client -c /etc/yandex/statbox-push-client/conf.d/push-client-billing.yaml --status 2>&1  | egrep "(':|last_send_time|last_commit_time|commit delay|lag)"
[Jul 08 09:15:19.980917] myt1-ct5-3.cloud.yandex.net [3767.4290]:  '/var/lib/yc/network-billing-collector/accounting/accounting.log':
[Jul 08 09:15:19.980994] myt1-ct5-3.cloud.yandex.net [3767.4290]:    commit delay: 759832
[Jul 08 09:15:19.980999] myt1-ct5-3.cloud.yandex.net [3767.4290]:    lag: 0
[Jul 08 09:15:19.981067] myt1-ct5-3.cloud.yandex.net [3767.4290]:    last_commit_time: 1624975220 (29.06.2021-14.00.20)
[Jul 08 09:15:19.981073] myt1-ct5-3.cloud.yandex.net [3767.4290]:    last_send_time: 1625735716 (08.07.2021-09.15.16)
[Jul 08 09:15:19.981091] myt1-ct5-3.cloud.yandex.net [3767.4290]:  '/var/lib/yc/compute-node/accounting/accounting.log':
[Jul 08 09:15:19.981146] myt1-ct5-3.cloud.yandex.net [3767.4290]:    commit delay: 24
[Jul 08 09:15:19.981151] myt1-ct5-3.cloud.yandex.net [3767.4290]:    lag: 0
[Jul 08 09:15:19.981228] myt1-ct5-3.cloud.yandex.net [3767.4290]:    last_commit_time: 1625735693 (08.07.2021-09.14.53)
[Jul 08 09:15:19.981236] myt1-ct5-3.cloud.yandex.net [3767.4290]:    last_send_time: 1625735717 (08.07.2021-09.15.17)
```
5. Можно проверить статус на принимающей стороне: [Logbroker UI](https://lb.yandex-team.ru/).
   - Выбираем нужный account (в устаревшей терминологии это `ident`, его можно посмотреть в конфиге (например, `/etc/yandex/statbox-push-client/conf.d/push-client-billing.yaml` - `ident: yc-df-pre`)
   - Выбираем нужный topic (в том же конфиге, например, `log_type: billing-compute-instance`)
6. Новую версию мониторинга можно запустить с ключом отладки: `/home/monitor/agents/modules/push-client.py --debug`

## Ссылки
- [Документация push-client](https://wiki.yandex-team.ru/logbroker/docs/push-client/)
- [Infra status page](https://nda.ya.ru/t/9er7UiVw3mCjXU) - можно проверить, ведутся ли работы в Logbroker
- [Чат поддержки Logbroker](https://telegram.me/joinchat/BvmbJED8I3aGqqtk_ssAbg)
- [CLOUD-74217](https://st.yandex-team.ru/CLOUD-74217)
