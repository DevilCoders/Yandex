# lb-node-ping

**Значение:** Сервис lb-node (control plane балансера, настройщик vpp/gobgp) не работает полностью или частично.
**Воздействие:** Не обновляются балансировочные правила в vpp/gobgp. Не выключаются мёртвые реалы, не удаляются старые хосты - проливаем трафик. Не добавляются новые правила, не включаются живые реалы - перегружаем сервисы клиентов.
**Что делать:** Ошибки вида "Connection refused"
1. Посмотреть, работает ли vpp:
`sudo systemctl status vpp`
2. Если vpp активен, то попробовать запустить lb-node:
`sudo systemctl restart yc-loadbalancer-node`
3. Если это не помогает, посмотреть в логи:
`sudo journalctl -u yc-loadbalancer-node -S -30m`
4. Если vpp не активен, запустить vpp - lb-node не имеет смысла работать без vpp:
`sudo systemctl start vpp`
Для других ошибок также проверяем зависимости сервисов. Дополнительно смотрим:
`curl -s 0x0:4050/ping`
тут есть дополнительная отладочная инфа:
* `daemon.started` - не удалось провести начальную настройку, тут смотреть в логи, ждать/рестартовать
* `state.handled` - не удалось получить текущее состояние от lb-ctrl. Можно проверить lb-ctrl, что они думают по этому поводу:
`pssh run -p10 'curl -s 0x0:4050/debug/watcher | grep node' C@cloud_preprod_ylb_stable_lb-ctrl`
* `state.vpp_call` - показывает, что не удалось с первого раза записать в vpp. Можно погасить проверку рестартом yc-loadbalancer-node, либо через специальную ручку: `curl 0x0:4050/debug/vpp/clear-err`
Смотреть в логи:
`sudo journalctllb -u yc-loadbalancer-node -S -30m`
