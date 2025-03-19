# push-client

**Значение:** Статус отгрузки данных push-client'ом в LogBroker
**Воздействие:** Не доставляются метрики из сервисов балансера/billbro в биллинг, у пользователей задержка метрик потребления ресурсов или нули
**Что делать:**
1. Ищем ошибки в логе push-client'а: `sudo grep -C5 -i err /var/log/statbox/logsync.log`
или для billbro `sudo grep -C5 -i err /var/log/statbox/billbro.log`
2. Смотрим за динамикой задержки доставки логов: `push-client -c /etc/yandex/statbox-push-client/conf.d/push-client-logsync.yaml --status |& grep -i lag` или для billbro `push-client -c /etc/yandex/statbox-push-client/conf.d/push-client-billbro.yaml --status |& grep -i lag`
3. Смотрим в графики количества свободного места в logshatter'ном clickhouse: https://console.cloud.yandex.ru/folders/b1gcc8kvpne1dl3f8i0q/managed-clickhouse/cluster/c9qhej15oj834d30taek?section=monitoring
4. смотрим в статус на странице logbroker https://lb.yandex-team.ru/logbroker/accounts/yandexcloud?page=browser&type=account&sortOrder=%22default%22 [ylb_prod_stable_logs](https://lb.yandex-team.ru/logbroker/accounts/yandexcloud/ylb-prod-log?page=statistics&type=topic&tab=writeMetrics&shownTopics=all%20topics&metricsFrom=1591091740334&metricsTo=1591178140334&sortOrder=%22default%22)/[ylb_preprod_stable_logs](https://lb.yandex-team.ru/logbroker/accounts/yandexcloud/ylb-preprod-stable-log?page=statistics&type=topic&tab=writeMetrics&shownTopics=all%20topics&metricsFrom=1591092267921&metricsTo=1591178667922&sortOrder=%22default%22) - логи балансера из journald
5. Если лаг растёт, рестартуем push-client `systemctl restart statbox-push-client.service`
Про logbroker можно писать в очереди [LBOPS](https://st.yandex-team.ru/LBOPS/) и [LOGBROKER](https://st.yandex-team.ru/LOGBROKER)
<[
- Помощь: https://wiki.yandex-team.ru/logbroker/help/
- Призвать дежурного /logbroker@YaIncBot
- Тел. дежурного 1144, вне офиса +7(495)739-70-00
- Status page: https://infra.yandex-team.ru
- Интерфейс: https://lb.yandex-team.ru
- Ссылка на этот чат: https://telegram.me/joinchat/BvmbJED8I3aGqqtk_ssAbg
- Q (только если Telegram отключат): https://q.yandex-team.ru/#/join/08a19f87-de45-4163-b37e-c1692e32455f
- График дежурств: https://nda.ya.ru/3SPKe5
- Форма для заявки о настройке ЛБ https://forms.yandex-team.ru/surveys/7833/
- Вопросы по настройке поставки и Logfeller: https://t.me/joinchat/AAAAAELJsROI0o5DV6mycQ
- Факапочный чатик Logfeller: https://t.me/joinchat/AAAAAEF_LLKllCFkJJ_0EA
- Вопросы по Logfeller и данным https://t.me/joinchat/AAAAAELJsROI0o5DV6mycQ
]>
Также см. заметки к проверке [logsync](https://wiki.yandex-team.ru/cloud/devel/loadbalancing/monitorings/#logsync).
