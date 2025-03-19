[Алерт e2e-tests-permnet-dns в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3De2e-tests-permnet-dns)

Как и [e2e-tests-permnet-connectivity](e2e-tests-permnet-connectivity.html) используют permnet-виртуалку на гипервизоре. Тесты запускаются на каждой **compute-node** раз в **120сек**. Каждый тест отписывает дополнительное сырое событие в juggler, по ним можно построить отдельные агрегаты, чтобы сразу знать какая часть DNS сломалась.

- **test_ipv6_dns_int_records** — проверяет, что на виртуалке получается отрезолвить AAAA и PTR (v6) записи самой виртуалки

- **test_ipv4_dns_int_records** — проверяет, что на виртуалке получается отрезолвить A и PTR (v4) записи самой виртуалки

- **test_ipv4_dns_ext_records** — проверяет, что на виртуалке получается отрезолвить A и PTR (v4) записи dns.yandex.ru/77.88.8.8

- **test_mdb_dns_int_records** — проверяет, что на виртуалке получается отрезолвить A и AAAA записи внутрисетевых mdb-инстансов а также их CNAME-алиасов (то есть CNAME-записи, ссылающиеся на A/AAAA-записи)