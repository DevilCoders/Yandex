# hc-ctrl http-ping

**Значение:** Сервис yc-healthcheck-ctrl не работает (упал/постопан/не проинициализировался до конца).
**Воздействие:** Если все hc-ctrl в зоне горят этой проверкой, то в зоне не работает изменение правил балансировки в data plane и не работают задачи от config plane и hcaas, не обновляются статусы у реалов, пользователи и IG видят неактуальные статусы
**Что делать:** Посмотреть логи:
`sudo journalctllb -u yc-healthcheck-ctrl -S -30m`
Стартовать сервис:
`sudo systemctl start yc-healthcheck-ctrl`
Дополнительная инфа:
`curl -s 0x0:4050/debug/targets` - привязка target'ов и hc-node и hc-ctrl по мнению контроллера и вспомогательная инфа
`curl -s 0x0:4050/debug/ydb/status` - инфа по балансировке ydb
