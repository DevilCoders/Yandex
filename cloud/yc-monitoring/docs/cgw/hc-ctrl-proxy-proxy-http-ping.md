# hc-ctrl-proxy proxy-http-ping

**Значение:** Сервис yc-healthcheck-ctrl-proxy не работает (упал/постопан/не проинициализировался до конца)
**Воздействие:** Если все прокси в инсталяции упадут - перестанет работать hcaas и IG будут видеть неактуальные статусы
**Что делать:** Посмотреть логи:
`sudo journalctllb -u yc-healthcheck-ctrl-proxy -S -30m`
Стартовать сервис:
`sudo systemctl start yc-healthcheck-ctrl-proxy`
Дополнительная инфа:
`curl -s 0x0:4060/debug/grpc/client` список подключёных  hc-ctrl'ов
