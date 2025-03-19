# walle_fs_check

**Значение:** Проблемы с дисками
**Воздействие:** На машине могут отвалиться диски со всеми вытекающими
**Что делать:** Поменять 0 0 на 1 1 в /etc/fstab согласно [инструкции](https://wiki.yandex-team.ru/cloud/compute/duty/#kogdamashinaeshhezhiva); далее ` touch /forcefsck && reboot `; далее вернуть 0 0
