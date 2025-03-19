# hc-ctrl http-ping

**Значение:** сервис yc-healthcheck-ctrl не может обновить состояние правил.
**Воздействие:** пользователи не могут добавить/удалить/изменить настройки healthcheck'ов, новые target'ы не становятся healthy.
**Что делать:** Посмотреть логи:
`sudo journalctllb -u yc-healthcheck-ctrl -S -30m`
Стартовать сервис:
`sudo systemctl start yc-healthcheck-ctrl`
Дополнительная инфа:
`curl -s 0x0:4050/debug/ydb` - инфа по балансировке ydb
Покрутить в конфиге параметры:
`target-polling-freq` чтобы уменьшить частоту поллинга (если ydb плохо).
`target-polling-timeout` и одновременно `ydb.request-timeout` и `ydb.operation-timeout` чтобы увеличить таймаут на выполнение чтения из базы. После изменений рестартовать сервис.

