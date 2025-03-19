# cgw-fibsync-v4/cgw-fibsync-v6

Проверяет воспроизведение CLOUD-16040, CLOUD-37017.
Может загораться при рестарте vpp на балансерах из-за низкой скорости прокачивания маршрутов до vpp.
На проверку **НЕ** настроен авторекавери.
Также может загораться, если пользователь настроил cic так, что на одном cicbr присутстуют больше 1 vrf, которые импортируются на стороне cgw-dc. Таких пользователей можно наказывать, исключая их из мониторинга [по инструкции](https://wiki.yandex-team.ru/cloud/devel/cloudgate/interconnect/#iskljuchenieupstreamcicizmonitoringavsluchaepodkljuchenijaodnojjploshhadkisneskolkimivrf)
Проверяется в рамках `cgw-known-bugs`
