[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-node-logging)

## compute-node-logging

Мониторит время записи логов.

## Подробности

Проверка появилась в результате [CLOUD-88210](https://st.yandex-team.ru/CLOUD-88210), где было замечено, что запись в лог становится бутылочным горлышком в наших сервисах.

Description всегда имеет вид `Write duration: q0.5: 264.206µs, q0.9: 641.655µs, q0.99: 1.467289ms` и переходит в `WARN`/`CRIT` в зависимости от текущих значений квантилей (пороги срабатывания захардкожены в коде).

Помимо Juggler-проверки есть еще гистограмма `log_write_duration` в Solomon, с помощью которой можно получить исторические данные.

## Диагностика

На данный момент проверка не звонящая и работает в экспериментальном режиме с порогами, взятыми с потолка. В случае срабатывания в первую очередь стоит оценить по-сервисный объем логов вот такой командой:
```bash
pssh HOST 'sudo journalctl -S-1h -o json | jq -r "._SYSTEMD_UNIT // .SYSLOG_IDENTIFIER // ._COMM // ._PID"' | sort | uniq -c | sort -nr
```

## Ссылки
- [CLOUD-88210](https://st.yandex-team.ru/CLOUD-88210)
