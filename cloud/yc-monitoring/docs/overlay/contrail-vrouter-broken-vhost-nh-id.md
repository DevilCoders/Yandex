[Алерт contrail-vrouter-broken-vhost-nh-id в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-vrouter-broken-vhost-nh-id)

## Что проверяет

- Что vhostX-интерфейс корректно настроен (влияет на работу Metadata Proxy).

## Если загорелось

1. Перезапустить агента
```
sudo safe-restart --force contrail-vrouter-agent
```

2. Скопировать дамп из директории `/var/lib/yc/yc-contrail-monitor-vrouter/*contrail-vrouter-broken-vhost-nh-id*` на
`dora.sec.yandex.net`, приложить ссылку в тикет https://st.yandex-team.ru/CLOUD-53108
