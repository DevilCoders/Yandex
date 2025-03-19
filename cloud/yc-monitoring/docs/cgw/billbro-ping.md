# billbro-ping

**Значение:** Сервис yc-billbro (биллинг балансера) не работает полностью или частично.
**Воздействие:** Не биллим пользовательский трафик/autoscaler'р у instance group не получает метрики с балансера.
**Что делать:** Ошибки вида "Connection refused"
1. Посмотреть, работает ли vpp:
`sudo systemctl status vpp`
2. Если vpp активен, то попробовать запустить billbro:
`sudo systemctl restart yc-billbro`
3. Если это не помогает, посмотреть в логи:
`sudo journalctl -u yc-billbro -S -30m`
4. Если vpp не активен, запустить vpp - billbro не имеет смысла работать без vpp:
`sudo systemctl start vpp`
Для других ошибок также проверяем зависимости сервисов. Дополнительно смотрим:
`curl -s 0x0:4444/ping`
тут есть дополнительная отладочная инфа:
* `daemon.started` - не удалось провести начальную настройку, тут смотреть в логи, ждать/рестартовать
* `rotate-push-file` - не удалось ротировать файл лога `/var/log/billbro/billing.json` или параметр `dump-file` в конфиге. Смотреть в логи, проверять права на запись в директорию, свободное место на диске.
* `autoscale` - не удаётся отправить метрики в autoscaler (параметр `autoscale-target` в конфиге). Тут смотреть в логи и общаться с /duty instancegroup или [rurikk@](https://staff.yandex-team.ru/rurikk)]).
* `ycnat.getstats` - не удаётся забрать данные из vpp. Нужно проверить, жив ли vpp (`sudo systemctl status vpp` , `sudo vppctl sh version`), проверить права на `/dev/shm/*` для billbro. Подождать тоже может помочь - первый запрос может занимать некоторое время.
