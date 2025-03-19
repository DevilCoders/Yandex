# vpp-modules

**Значение:** проблема при загрузке модулей vpp
**Воздействие:** может не работать какая-то функциональность в vpp
**Что делать:** смотреть лог vpp на предмет строк вида `Failed to load plugin.*` флаг о наличии проблемы создается сервисом check-vpp-modules.service, который стартует после сервиса yavpp-configured
Если в `/usr/lib/bpp_plugins` есть плагин, на который ругается vpp, помогает рестарт vpp после создания следующего файла:
```
$ cat /etc/systemd/system/vpp.service.d/00-yclb-environment.conf
[Service]
Environment=LD_LIBRARY_PATH=/usr/lib/vpp_plugins
```
