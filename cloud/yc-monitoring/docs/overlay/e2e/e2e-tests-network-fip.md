[Алерт e2e-tests-network-fip в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3De2e-tests-network-fip)

Тесты, приведенные ниже, запускают виртуалку **с белым адресом** (с FIP-ом). Запускается одна виртуалка на все тесты.

Все тесты запускаются **под per-az kikimr-lock'ом**. Тесты запускаются на **oct-head** раз в **300сек**. Если нода не захватила lock — в juggler возвращается 1 и статус «Warn: Skipped all the tests». Поскольку проверка «ездит» между хостами, в агрегате стоит `hold_crit: 300`, чтобы желтые значения не замазали ошибку.

- **test_ipv4_connectivity** — заходим на виртуалку, проверяем, что с нее пингуется по ipv4 набор хостов (задается в конфиге, по умолчанию `yandex.ru,mail.ru,google.com,amazon.com,baidu.com,qq.com,web.de,t-online.de,bild.de`). Загорается красным только если **все хосты** не пингуются.

- **test_martian_connectivity** — берем ipv4 адрес сайта https://ipv4-internet.yandex.net/, c виртуалки идем в него по ipv4 и проверяем, что в яндексе мы видны через ip-шник из марсианской сети 198.18.0.0/15. Сеть и урл задаются в конфиге. Если не работает сервис [https://ipv4-internet.yandex.net/](https://ipv4-internet.yandex.net/), можно обратиться к кому:dench либо пойти в [очередь INTERNETOMETR](https://st.yandex-team.ru/INTERNETOMETR/)

- **test_ipv4_yandex_extra** — проверяет связность до сателитов яндекса (`kassa.yandex.ru` и `payment.yandex.net`). Если все недоступны, загорается красным.