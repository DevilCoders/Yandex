[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=Compute+node+%28go%29+hanging+tasks), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-node-hanging-tasks%26namespace%3Dycloud)

## compute-node-hanging-tasks
Загорается, если таска на compute-node не успела завершиться за разумное время.

## Подробности
Тип зависшей таски выводится в аннотациях алерта, например: `hanging_task_types: [migrate-instance]`.

![](https://jing.yandex-team.ru/files/simonov-d/Screenshot%20from%202021-07-07%2017-39-34.png)

Таймаут вычисляется в зависимости от типа таски и характеристик инстанса (например, таймаут увеличивается при живой миграции, при остановке инстанса с shutdown grace time или при запуске инстанса с преаллокацией памяти).

## Диагностика
- Заходим на проблемную compute-node
- Ищем проблему в логах, вычисляем, с каким инстансом она связана (`[i=...]`):
```
(TESTING)simonov-d@myt1-ct5-1:~$ sudo journalctl -u yc-compute-node --since -1hour | grep "hanging task"
Jul 07 12:20:15 myt1-ct5-1.cloud.yandex.net yc-compute-node[55976]: [taskmetrics/taskmetrics.go:119] W: [i=bl8nntmp6hqrtdv8t0ag] [r=4e5e74ea-7d17-4510-9d01-224df332963c] [u=4e5e74ea-7d17-4510-9d01-224df332963c] Report "migrate-instance" hanging task.
```
- Смотрим лог по проблемному инстансу в окрестности ошибок
```
(TESTING)simonov-d@myt1-ct5-1:~$ sudo journalctl -u yc-compute-node --since -1hour INSTANCE_ID=bl8nntmp6hqrtdv8t0ag | grep "hanging task" -B 100
Jul 07 12:19:15 myt1-ct5-1.cloud.yandex.net yc-compute-node[55976]: [qmp/connection.go:603] D: [i=bl8nntmp6hqrtdv8t0ag] [rt=2.6ms] QMP response: {"return": {}}
Jul 07 12:19:15 myt1-ct5-1.cloud.yandex.net yc-compute-node[55976]: [qmp/connection.go:212] I: [i=bl8nntmp6hqrtdv8t0ag] [r=dd2cb617-dd52-4a38-b979-37e8961e43ea] [o=bl89biplpmqio6nmbbfh] QMP query has completed.
Jul 07 12:19:15 myt1-ct5-1.cloud.yandex.net yc-compute-node[55976]: [qmp/connection.go:159] I: [i=bl8nntmp6hqrtdv8t0ag] [r=dd2cb617-dd52-4a38-b979-37e8961e43ea] [o=bl89biplpmqio6nmbbfh] QMP query: x-block-latency-histogram-set({"device":"node-name-bl83jkqbc7lsivb9mnds","boundaries":[1000000,2000000,5000000,10000000,20000000,50000000,100000000,200000000,500000000,1000000000,2000000000,5000000000]})...
...
```

## Возможные причины
- `hanging_task_types: [start-instance]` - фрагментация памяти на хосте: [CLOUD-75155 Побороть/снизить влияние высокой фрагментации памяти на хосте](https://st.yandex-team.ru/CLOUD-75155). При этом в логе `yc-compute-node` будет большая задержка между сообщениями:
```
Dec 05 17:41:32 <HOST> yc-compute-node[828966]: [qmp/connection.go:286] D: [i=a7lds6q3bv14ftaub4g2] Connecting to QMP socket...
...
Dec 05 17:43:47 <HOST> yc-compute-node[828966]: [qmp/connection.go:473] D: [i=a7lds6q3bv14ftaub4g2] Got QMP greeting message: {"QMP": {"version": {"qemu": {"micro": 0, "minor": 12, "major": 2}, "package": "Debian 1:2.12.0-53"}, "capabilities": []}}.
```
- `hanging_task_types: [start-instance]` - долгое добавление пользователя в систему (до нескольких минут) [CLOUD-87974: Работа с пользователями периодически замедляет подхватывание инстанса на ноде](https://st.yandex-team.ru/CLOUD-87974). В логе вы увидите:
```
Feb 03 08:29:09 <HOST> yc-compute-node[552684]: [sys/process.go:60] D: [i=fhmp0r1uptkp7uqftba3] Executing `/usr/sbin/useradd --system i-fhmp0r1uptkp7uqftba3 -g i-fhmp0r1uptkp7uqftba3 --home-dir /var/lib/yc/compute-node/instances/fhmp0r1uptkp7uqftba3 -M --shell /usr/sbin/nologin -G yc-instances`...
Feb 03 08:30:36 <HOST> yc-compute-node[552684]: [sys/process.go:64] D: [i=fhmp0r1uptkp7uqftba3] `/usr/sbin/useradd --system i-fhmp0r1uptkp7uqftba3 -g i-fhmp0r1uptkp7uqftba3 --home-dir /var/lib/yc/compute-node/instances/fhmp0r1uptkp7uqftba3 -M --shell /usr/sbin/nologin -G yc-instances` has completed.
```
- один из дисков в RAID-е (mirror) выходит из строя. Посмотреть на [график](https://solomon.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod&service=sys&l.path=%2FIo%2FDisks%2FWriteWaitMillisec&l.host=vla04-ct21-2&graph=auto&stack=false&b=1w&e=) (выбрать правильный кластер и ноду). Если есть дисбаланс между двумя дисками в mirror-е и значения зашкаливают за единицы секунд, призвать Infra. Может быть полезен [bpftrace-скрипт](https://paste.yandex-team.ru/7123363/text) от Core, запускать `sudo bpftrace script.txt`.
```
(PROD)simonov-d@vla04-ct33-11:~$ mount | grep md1
/dev/md1 on /var type ext4 (rw,relatime,data=ordered)

(PROD)simonov-d@vla04-ct33-11:~$ cat /proc/mdstat | grep md1 -A2
md1 : active raid1 sda4[0] sdd4[1]
      314441728 blocks super 1.2 [2/2] [UU]
      bitmap: 3/3 pages [12KB], 65536KB chunk
```
