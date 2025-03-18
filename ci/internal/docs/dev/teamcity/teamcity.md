# Документация о TeamCity
* https://wiki.yandex-team.ru/users/aokhotin/vremennaja/teamcity-info/
* https://wiki.yandex-team.ru/teamcity/admin

## YC
Каталог **cap**

## Продовая инсталляция

**Интерфейс:** https://teamcity.yandex-team.ru/

**HW Master:** root@teamcity-v-1.ft.yandex.net

**HW Slave:** root@teamcity-h-1.ft.yandex.net

### Обновление настроек плагинов
Можно изменить прямо из интерфейса:
https://teamcity.yandex-team.ru/admin/admin.html?item=diagnostics&tab=dataDir&file=config/internal.properties


#### Добавление префиксов arc для сканирования
```properties
yandex.arc.allowed.branch.prefixes=trunk,users,releases,tags/groups/sdg,tags/releases/crm
```


## Тестовая инсталляция мобилок

**Интерфейс:** https://teamcity-testing.ft.yandex.net/

Логин и пароль для входа: https://yav.yandex-team.ru/secret/sec-01dt6e92rrwe0enn0y9wqa649w/explore/versions

**Хост:** root@teamcity-testing-f-1.ft.yandex.net

**Мониторинг:** https://grafana.qatools.yandex-team.ru/d/5ahaVedZk/teamcity-server-health?orgId=1&var-servername=teamcity-testing-f-1_ft_yandex_net&var-metrichost=teamcity-testing-h-1.ft.yandex.net&refresh=1m

**Агенты:** запущены в Docker контейнерах на том же хосте, см. `docker ps -a`

### Обновление сервиса

https://wiki.yandex-team.ru/teamcity/admin/#obnovlenieservera - в отличие от прода, у нас нет MySQL slave-а, поэтому обновление выглядит проще. Нужно выключить сервис, переключить симлинк с конфигами, а потом запустить сервис снова.

В тестинге база данных лежит в PostgreSQL, а не в MySQL!

Бэкапить данные не нужно (бэкап всё равно, скорее всего, не будет работать).


## Prestable (развернутый бэкап прода)

**Интерфейс:** https://teamcity-testing.yandex-team.ru/

**Хост:** root@teamcity-testing-h-1.ft.yandex.net

### Обновление сервиса

См. обновление для "Тестовой инсталляции мобилок".

**Восстановление из бэкапа:** см. `/etc/cron.d/teamcity-backup-restore`

**Существующий DataTransfer Link:** есть настроенный линк с продовой базы на тестовую: https://yc.yandex-team.ru/folders/fooj2l6257l5rd9deioq/data-transfer/transfer/dttr081ivccdidukkg3u/view

Необходимо убедиться, что он выключен, в противном случае восстановить базу из бэкапа не получится: `Cannot proceed with 'restore' command: Target database is not empty. Found 1 tables: __table_transfer_progress.`

**Бэкап раворачивается 14 часов.**
