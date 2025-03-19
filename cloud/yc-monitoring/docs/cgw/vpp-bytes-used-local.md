# vpp-bytes-used-local

**Значение:** проверяет, что vpp использует не больше положенного количества памяти. Если лимит превышен, на lb-node запускается авторекавери.
**Воздействие:** при падении будем дропать часть трафика, которая попала на эту vpp
**Что делать:** Рестартуем vpp:
1. Смотрим, что остальные lb-node в зоне живые. Если есть соседи с не-зелёными мониторингами, то ждём позеленения, если проверка жёлтая. Если красная - на усмотрение дежурного.
2. Рестартуем проблемную lb-node: `sudo yavpp-restart` .
3. Дожидаемся зелёных мониторингов на проблемной lb-node. Как правило, последний мониторинг, которого ждём - `drain-status`. Проверяем в `monrun -w` и `jurun -c` .
4. Проверяем соседние машины в зоне на предмет возможного скорого падения: [prod](https://solomon.cloud.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod_ylb&service=vsop&l.host=!cluster&l.name=vpp-bytes-used&graph=auto&stack=false) [preprod](https://solomon.cloud.yandex-team.ru/?project=yandexcloud&cluster=cloud_preprod_ylb&service=vsop&l.host=!cluster&l.name=vpp-bytes-used&graph=auto&stack=false) - выбираем в выпадушке dc загоревшейся проверки и смотрим, кто ещё может упасть и по той же схеме рестартуем их. Повторяем упражнение для остальных dc.
