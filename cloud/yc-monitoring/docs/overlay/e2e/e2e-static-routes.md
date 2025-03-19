[Алерт e2e-static-routes-new в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3De2e-static-routes-new)

[Алерт e2e-static-routes-perm в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3De2e-static-routes-perm)

Бывает `e2e-static-routes-new` и `e2e-static-routes-perm`.

- запускаются с **oct-голов** раз в **900сек** (new) и **120сек** (perm)

- проверяют функциональность [таблиц статических маршрутов](https://st.yandex-team.ru/CLOUD-15625):

  - `e2e-static-routes-new` — для новых инстансов/сетей/route-table-ов

    - работают **за KiKiMR Lock**

    - перед началом теста удаляются сущности, созданные на предыдущем запуске

  - `e2e-static-routes-perm` — для существующих инстансов/сетей/route-table-ов

    - работают без KiKiMR Lock

    - все сущности создаются один раз (при первом запуске)

- создают сеть, подсеть, 2 инстанса, route table (с маршрутом до второго инстанса через левый IP-адрес)

- пингуют с первого инстанса второй:

  - сначала по обычному IP (чтобы проверить, что связность вообще есть)

  - затем по адресу, смаршрутизированному через route-table

- загораются **красным** только если все проверки прошли, кроме пинга по маршрутизированному через route-table IP. В противном случае загораются **жёлтым**.