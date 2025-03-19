[Алерт contrail-backup в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-backup)

## Что проверяет

Успешность бэкапа конфигурационной базы контрейла из кассандры в S3. См. `sudo crontab -l`.

## Если загорелось

- Обычно ждёт утра или начала рабочего дня

- На любой голове посмотреть, какого бэкапа не хватает:

  `pssh C@cloud_prod_oct_vla 'sudo yc-contrail-backup ls-s3 2>&1 | tail -n5'`

  Пример вывода:

```
2021-05-17 13:24:11,501 INFO    20210517-051157-myt.tgz.gpg
2021-05-17 13:24:11,501 INFO    20210517-051235-vla.tgz.gpg
2021-05-17 13:24:11,501 INFO    20210517-051431-sas.tgz.gpg
2021-05-17 13:24:11,501 INFO    20210517-101330-myt.tgz.gpg
2021-05-17 13:24:11,501 INFO    20210517-101437-sas.tgz.gpg
```

  не обращать внимание на время слева, только на имя файла! Тут видим, что отсутствует бэкап vla в 10 часов.

- Бэкапы запускаются с разных голов в разное время, нужно выяснить, с какой запускается в 10 часов:

  `pssh run "sudo crontab -l | grep contrail-backup" C@cloud_prod_oct_vla`

```
9 5 * * * /usr/bin/yc-contrail-backup cleanup backup --cluster vla --host oct-vla2.v4.svc.cloud.yandex.net >>/var/log/cassandra/contrail-backup.log 2>&1

oct-vla5.svc.cloud.yandex.net:
OUT[0]:
9 20 * * * /usr/bin/yc-contrail-backup cleanup backup --cluster vla --host oct-vla5.v4.svc.cloud.yandex.net >>/var/log/cassandra/contrail-backup.log 2>&1

oct-vla3.svc.cloud.yandex.net:
OUT[0]:
9 10 * * * /usr/bin/yc-contrail-backup cleanup backup --cluster vla --host oct-vla3.v4.svc.cloud.yandex.net >>/var/log/cassandra/contrail-backup.log 2>&1

oct-vla1.svc.cloud.yandex.net:
OUT[0]:
9 0 * * * /usr/bin/yc-contrail-backup cleanup backup --cluster vla --host oct-vla1.v4.svc.cloud.yandex.net >>/var/log/cassandra/contrail-backup.log 2>&1

```

  видно, что нас интересует oct-vla3

- идём на oct-vla3, смотрим лог:

  `pssh oct-vla3.svc.cloud.yandex.net 'tail -n20 /var/log/cassandra/contrail-backup.log'`

- можно запустить бэкап вручную, получаем команду из crontab:

  `pssh oct-vla3.svc.cloud.yandex.net 'sudo crontab -l | grep /usr/bin/yc-contrail-backup | cut -d" " -f6-'`, проверяем глазами и

  `pssh oct-vla3.svc.cloud.yandex.net 'sudo bash -c "<команда>"'`

- логи в `/var/log/cassandra/contrail-backup.log`

- `yc-contrail-backup ls` — посмотреть содержимое локальных бэкапов

- `yc-contrail-backup ls-s3` — посмотреть содержимое S3-бакета с бэкапами