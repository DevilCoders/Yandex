[Алерт contrail-control-routes-divergence в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-control-routes-divergence)

Проверяет, что от всех типов CGW на contrail-control-ы в данной AZ пришло одинаковое количество маршрутов. По каждой группе (типу) cgw: cgw-ipv4, cgw-ipv6, cgw-dc, cgw-nat, ...

Защищает нас от повторения [CLOUDINC-1623](https://st.yandex-team.ru/CLOUDINC-1623) из-за проблем на CGW. Также замечает [CLOUD-70780](https://st.yandex-team.ru/CLOUD-70780)

Загорается:

- CRIT, если маршрутов вообще нет ни на одном contrail-control от данной группы

- CRIT, если разница между max-линией (кол-ва маршрутов) и min-линией за последние 5 минут больше 5% (среднее значение)

- WARN, если разница от 1% до 5%

Что делать:

- призывать `/duty cgw` (скорее всего, требуется рестарт cgw). Если `/duty cgw` не знает, как чинить, скажите им, что прошлая инструкция выглядела так: `pssh run 'gobgp neigh 10.0.163.21 softresetin; yavpp-disable-announces --disable-group reflector; yavpp-disable-announces --enable-group reflector' C@cloud_${ENV}_cgw-dc_${DC} C@cloud_${ENV}_cgw-nat_${DC} C@cloud_${ENV}_cgw-ipv6_${DC} C@cloud_${ENV}_cloudgate_${DC}`

- может пригодиться детализация по кол-ву маршрутов с конкретных CGW:

```

pssh run -p5 "yc-contrail-introspect-control neigh | grep cgw-ipv6" C@cloud_prod_oct_vla

```

(поменять `cgw-ipv6` и `C@cloud_prod_oct_vla` на актуальные значения)

Может [моргать](https://bb.yandex-team.ru/projects/CLOUD/repos/yc-monitoring/pull-requests/88/overview?commentId=1548416) при добавлении новых CGW (пока они не проросли в cluster-map на oct_heads).

[Тикет](https://st.yandex-team.ru/CLOUD-72125)
