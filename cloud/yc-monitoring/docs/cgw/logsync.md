# logsync

**Значение:** Сервис logsync не работает полностью или частично.
**Воздействие:** Не доставляются логи в logbroker/clickphite, не работают алерты настроенные на clikphite (процент ошибок по логам, например), теряем логи в yt'е.
**Что делать:** Ошибки вида: "lag Tms" - есть задержка доставки данных до logbroker.
1. Смотрим `tail -1000 /var/log/logsync` на предмет различных ошибок
2. Смотрим `tail -1000 /var/log/statbox/logsync.log` - вероятно, там будут сетевые проблемы по отправке данных в logbroker. Смотри контакты logbroker'а в инструкции к проверке https://wiki.yandex-team.ru/cloud/devel/loadbalancing/monitorings/#push-client .
3. Проверить задержку отправки логов push-client'ом:
`push-client -c /etc/yandex/statbox-push-client/conf.d/push-client-logsync.yaml --status |& grep -i lag`
4. Смотрим в графики количества свободного места в logshatter'ном clickhouse: https://console.cloud.yandex.ru/folders/b1gcc8kvpne1dl3f8i0q/managed-clickhouse/cluster/c9qhej15oj834d30taek?section=monitoring
5. смотрим в статус на странице logbroker https://lb.yandex-team.ru/logbroker/accounts/yandexcloud?page=browser&type=account&sortOrder=%22default%22 [ylb_prod_stable_logs](https://lb.yandex-team.ru/logbroker/accounts/yandexcloud/ylb-prod-log?page=statistics&type=topic&tab=writeMetrics&shownTopics=all%20topics&metricsFrom=1591091740334&metricsTo=1591178140334&sortOrder=%22default%22)/[ylb_preprod_stable_logs](https://lb.yandex-team.ru/logbroker/accounts/yandexcloud/ylb-preprod-stable-log?page=statistics&type=topic&tab=writeMetrics&shownTopics=all%20topics&metricsFrom=1591092267921&metricsTo=1591178667922&sortOrder=%22default%22) - логи балансера из journald
6. Если лаг растёт, рестартуем push-client `systemctl restart statbox-push-client.service`
7. Если забилось место в tmpdir и сервис не стартует: Удаление всех файлов в /mnt/logsync/temp не срабатывает. Нужно удалить директорию и пересоздать ее с правильными правами:
```
sudo rm -rf  /mnt/logsync/temp
sudo mkdir -p /mnt/logsync/temp
sudo chown yc-logsync:yc-logsync-operators /mnt/logsync/temp
sudo systemctl start yc-logsync.service
```
Про logbroker можно писать в очереди [LBOPS](https://st.yandex-team.ru/LBOPS/) и [LOGBROKER](https://st.yandex-team.ru/LOGBROKER)
[Logsync dashboard prod ](https://solomon.cloud.yandex-team.ru/?cluster=cloud_prod_ylb&service=logsync&host=hc-ctrl-rc1b-01&project=yandexcloud&env=prod&dashboard=yandexcloud-ylb-prod-logsync)
[Logsync dashboard preprod ](https://solomon.cloud.yandex-team.ru/?cluster=cloud_preprod_ylb&service=logsync&host=hc-node-rc1b-01&project=yandexcloud&env=preprod&dashboard=yandexcloud-ylb-preprod-logsync)
Также см. заметки к проверке [push-client](https://wiki.yandex-team.ru/cloud/devel/loadbalancing/monitorings/#push-client).
