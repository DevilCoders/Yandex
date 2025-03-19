# bpftrace-reporter-ping

**Значение:** Сервис bpftrace-reporter не работает полностью или частично.
**Воздействие:** Не собираются bpftrace метрики.
**Что делать:** Ошибки вида "Connection refused"
1. Попробовать запустить bpftrace-reporter:
`sudo systemctl restart yc-bpftrace-reporter`
3. Если это не помогает, посмотреть в логи:
`sudo journalctl -t yc-bpftrace-reporter -S -30m`
