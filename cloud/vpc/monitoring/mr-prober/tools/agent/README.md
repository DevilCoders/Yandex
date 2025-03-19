# Утилита `yc-mr-prober-agent`

Предназначена для задач дежурного или исследователя инцидентов на агентской виртуалке в Mr. Prober. Позволяет узнать
информацию о виртуалке, а также запустить пробер много раз в цикле, чтобы отладить проблему.

## Как запустить?

На виртуальных машинах, собранных из образа агента, можно воспользоваться обёрткой `yc-mr-prober-agent`:

```
andgein@vla04-ct7-27:~$ yc-mr-prober-agent info
Attaching to docker container with Mr. Prober Agent...

```

Эта обёртка очень простая — она подключается к докер-контейнеру `agent.service` и выполняет внутри него
команду `tools/agent/main.py "$@"`.

```bash
$ cat `which yc-mr-prober-agent`
#!/bin/bash

set -e

echo Attaching to docker container with Mr. Prober Agent...
sudo docker exec -it agent.service python /mr_prober/tools/agent/main.py "$@"
```

## Как запустить локально?

Как и другие проекты Mr. Prober — из корневой директории внутри настроенного virtualenv.

```bash
PYTHONPATH=. tools/agent/main.py info
```

Команда `info` выводит базовую информацию о виртуальной машине, кластере Mr. Prober и запускаемых проберах:

```
# PYTHONPATH=. tools/agent/main.py info
╭────────────────────────────────────────────────────────────────────────────────┬────────────────────────────────────────────────────────────────────────────────╮
│ Instance ID                                                                    │ a7lo2kfhojhe5olsfhh0                                                           │
│ Instance FQDN                                                                  │ vla04-ct7-27.meeseeks.prober.cloud-preprod.yandex.net                          │
│ Hostname from $HOSTNAME                                                        │ vla04-ct7-27.meeseeks.prober.cloud-preprod.yandex.net                          │
│                                                                                │                                                                                │
│ Logs                                                                           │ /var/log/mr_prober                                                             │
│ Prober logs                                                                    │ /var/log/mr_prober/probers/<prober-slug>                                       │
│                                                                                │                                                                                │
│ Cluster ID                                                                     │ 3                                                                              │
│ Cluster Name                                                                   │ meeseeks                                                                       │
│ Cluster Slug                                                                   │ meeseeks                                                                       │
│                                                                                │                                                                                │
│ Probers:                                                                       │                                                                                │
│ 1. network-ping-8.8.8.8 (network-ping-8.8.8.8)                                 │ runs every 30 seconds with timeout 10 seconds, policy of logs uploading: FAIL  │
│ 2. [DNS] Resolve external domains (dns-resolve-external-hostnames)             │ runs every 30 seconds with timeout 10 seconds, policy of logs uploading: FAIL  │
│ 3. [DNS] Resolve external *.mdb.yandexcloud.net (dns-resolve-special-mdb-host) │ runs every 30 seconds with timeout 10 seconds, policy of logs uploading: FAIL  │
│ 4. [DNS] Reverse resolve VM by itself in (dns-resolve-vm-self-reverse)         │ runs every 30 seconds with timeout 10 seconds, policy of logs uploading: FAIL  │
│ 5. [DNS] Resolve storage-int.mdb.yandex.net (dns-resolve-yandex-host)          │ runs every 30 seconds with timeout 10 seconds, policy of logs uploading: FAIL  │
│ 6. [DNS] Resolve yandex.ru via system resolver (dns-resolve-yandex-ru)         │ runs every 30 seconds with timeout 10 seconds, policy of logs uploading: FAIL  │
│ 7. [Metadata] Check /hostname (metadata-hostname)                              │ runs every 60 seconds with timeout 10 seconds, policy of logs uploading: FAIL  │
│ 8. [Metadata] Check /local-hostname (metadata-local-hostname)                  │ runs every 180 seconds with timeout 10 seconds, policy of logs uploading: FAIL │
│ 9. [Metadata] Check /macs/.../local-hostname (metadata-local-hostname-by-mac)  │ runs every 180 seconds with timeout 10 seconds, policy of logs uploading: FAIL │
│ 10. [Metadata] Check local-ipv4 (metadata-local-ipv4)                          │ runs every 180 seconds with timeout 10 seconds, policy of logs uploading: FAIL │
│ 11. [Metadata] Check macs/.../local-ipv4 (metadata-local-ipv4s-by-mac)         │ runs every 180 seconds with timeout 10 seconds, policy of logs uploading: FAIL │
│ 12. [Metadata] Check macs/.../mac (metadata-mac-by-mac)                        │ runs every 180 seconds with timeout 10 seconds, policy of logs uploading: FAIL │
│ 13. [DNS] Resolve VM by itself (dns-resolve-vm-self)                           │ runs every 30 seconds with timeout 10 seconds, policy of logs uploading: FAIL  │
╰────────────────────────────────────────────────────────────────────────────────┴────────────────────────────────────────────────────────────────────────────────╯
╭───────────┬────────────────────────────────┬─────────────────────────────────────────────╮
│ Interface │ Addresses                      │ Network Mask                                │
├───────────┼────────────────────────────────┼─────────────────────────────────────────────┤
│ lo        │ 127.0.0.1                      │ 255.0.0.0                                   │
│           │ ::1                            │ ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/128 │
│ eth0      │ 10.1.0.43                      │ 255.255.0.0                                 │
│           │ fe80::d20d:18ff:fe15:1f1c%eth0 │ ffff:ffff:ffff:ffff::/64                    │
│ eth1      │ 2a02:6b8:c0e:501:0:fc2c:0:1c6  │ ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/128 │
│           │ fe80::d21d:18ff:fe15:1f1c%eth1 │ ffff:ffff:ffff:ffff::/64                    │
│ docker0   │ 172.17.0.1                     │ 255.255.0.0                                 │
│           │ fd00::1                        │ ffff:ffff:ffff:ffff:ffff::/80               │
│           │ fe80::1%docker0                │ ffff:ffff:ffff:ffff::/64                    │
╰───────────┴────────────────────────────────┴─────────────────────────────────────────────╯
```

## Список проберов

Можно получить список доступных в агенте проберов с помощью

```
# yc-mr-prober-agent probers list
Attaching to docker container with Mr. Prober Agent...

 1. network-ping-8.8.8.8 (network-ping-8.8.8.8)                                 runs every 30 seconds with timeout 10 seconds, policy of logs uploading: FAIL
 2. [DNS] Resolve external domains (dns-resolve-external-hostnames)             runs every 30 seconds with timeout 10 seconds, policy of logs uploading: FAIL
 3. [DNS] Resolve external *.mdb.yandexcloud.net (dns-resolve-special-mdb-host) runs every 30 seconds with timeout 10 seconds, policy of logs uploading: FAIL
 4. [DNS] Reverse resolve VM by itself in (dns-resolve-vm-self-reverse)         runs every 30 seconds with timeout 10 seconds, policy of logs uploading: FAIL
 5. [DNS] Resolve storage-int.mdb.yandex.net (dns-resolve-yandex-host)          runs every 30 seconds with timeout 10 seconds, policy of logs uploading: FAIL
 6. [DNS] Resolve yandex.ru via system resolver (dns-resolve-yandex-ru)         runs every 30 seconds with timeout 10 seconds, policy of logs uploading: FAIL
 7. [Metadata] Check /hostname (metadata-hostname)                              runs every 60 seconds with timeout 10 seconds, policy of logs uploading: FAIL
 8. [Metadata] Check /local-hostname (metadata-local-hostname)                  runs every 180 seconds with timeout 10 seconds, policy of logs uploading: FAIL
 9. [Metadata] Check /macs/.../local-hostname (metadata-local-hostname-by-mac)  runs every 180 seconds with timeout 10 seconds, policy of logs uploading: FAIL
 10. [Metadata] Check local-ipv4 (metadata-local-ipv4)                          runs every 180 seconds with timeout 10 seconds, policy of logs uploading: FAIL
 11. [Metadata] Check macs/.../local-ipv4 (metadata-local-ipv4s-by-mac)         runs every 180 seconds with timeout 10 seconds, policy of logs uploading: FAIL
 12. [Metadata] Check macs/.../mac (metadata-mac-by-mac)                        runs every 180 seconds with timeout 10 seconds, policy of logs uploading: FAIL
 13. [DNS] Resolve VM by itself (dns-resolve-vm-self)                           runs every 30 seconds with timeout 10 seconds, policy of logs uploading: FAIL
```

## Запуск пробера

Одна из самых полезных функций утилиты `yc-mr-prober-agent` — возможность запустить конкретный пробер для отладки
проблемы. Например, если на инстансе хворает сеть и иногда загорется мониторинг пробера `network-ping`, то можно
запустить

```
# yc-mr-prober-agent probers run network-ping --runs 500 --exit-on-fail
```

Эта команда запустит пробер 500 раз и остановится при первой неудаче. Если ничего не указывать, то пробер запустится 1
раз. Дополнительно доступны флаги:

- `--no-prober-output` — не выводить stdout и stderr пробера,
- `--output-only` — выводить **только** stdout и stderr пробера, не выводить ничего лишнего,
- `--interval 0.5` — выдерживать паузу в 0.5 секунд между запусками.

## Справка

Полная справка, как всегда, доступна в `yc-mr-prober-agent --help` (или `PYTHONPATH=. tools/agent/main.py --help`).
