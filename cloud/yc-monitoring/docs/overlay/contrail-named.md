[Алерт contrail-named в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-named)

## Что проверяет

Сложная проверка, состоящая из нескольких частей. Проверяет корректность и внутреннее состояние сервиса `contrail-named`.

**Практически неактуальна на всех стендах**, так как переехали на Cloud DNS.

Выглядит примерно так: `contrail-named: OK (started 10 days ago), response: OK, zone files: OK (7325), reconfigs: OK, rollforward failures: OK, recursive clients: OK (34/7900/8000)`

Состоит из следующих частей:

- `contrail-named` — наличие процесса `contrail-named`

- `response` — ответ сервиса `contrail-named` на DNS-запрос (в процессе reconfig named-у допустимо не отвечать до 15 секунд)

- `zone files` — наличие на диске всех файлов зон, указанных в `/etc/contrail/dns/contrail-named.conf`. См. [CLOUD-16258](https://st.yandex-team.ru/CLOUD-16258). **Есть авторекавери**.

- `reconfigs` — наличие успешных named reconfig-ов (должен выполняться каждые 100 секунд) в journald-логе. Может моргнуть, если почистился journald-лог. Если что, смотри в `journalctl -u contrail-dns-reload -f`.

- `rollforward failures` — наличие сообщений `"journal out of sync with zone"` за последние 5 минут в `/var/log/contrail/contrail-named.log`. См. [CLOUD-18153](https://st.yandex-team.ru/CLOUD-18153). **Есть авторекавери**.

- `recursive clients` — превышение лимита по рекурсивным клиентам. 

## Если загорелось

- позовите [andgein](https://staff.yandex-team.ru/andgein) :-)

- `systemctl status contrail-named`

- логи в `/var/log/contrail/contrail-named.log`

- `sudo journalctl -u contrail-dns-reload -f`

- рестартить через `safe-restart --force contrail-dns`. Это рестартанёт и `contrail-dns`, и `contrail-named`, !!что приведёт к недоступности DNS на этой ноде в течение 10+ минут!. Поэтому на следующей голове можно делать только через 20-30 минут. Если знаешь, что делаешь, можно рестартовать только `systemctl restart contrail-named`. В этом случае он должен перезапуститься и сразу прочитать текущие конфиги, что позволит ему отвечать на запросы пользователей.