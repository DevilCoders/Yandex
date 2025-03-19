# lb-ctrl http-ping

**Значение:** Сервис yc-loadbalancer-ctrl не работает  (упал/постопан/не проинициализировался до конца).
**Воздействие:** Если все lb-ctrl в зоне горят этой проверкой, то в зоне не работает изменение правил балансировки в data plane и не работают задачи от config plane, не прогружаются правила в loadbalaner-node, у пользователей и IG не обновляется балансировка.
**Что делать:** Посмотреть логи:
`sudo journalctllb -u yc-loadbalancer-ctrl -S -30m`
Стартовать сервис:
`sudo systemctl start yc-loadbalancer-ctrl`
Дополнительная инфа:
`curl -s 0x0:4050/debug/watcher` - список target'ов, отгруженных в lb-node
`curl -s 0x0:4050/debug/grpc/client` - список подключёных hc-ctrl'ов
`curl -s 0x0:4050/debug/ydb/status` - инфа по балансировке ydb
