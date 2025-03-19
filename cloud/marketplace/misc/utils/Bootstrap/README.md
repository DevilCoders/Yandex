1. Создаём виртуалку с доступом в апи необходимой среды и во внутреннюю сеть (Virtualbox, DF vm, etc) со стоковой убунтой (проверить ping6 2a02:6b8:0:3400:0:587:0:81)
2. Кладём ключик от robot-marketplace в ~/.ssh/secdist (https://wiki.yandex-team.ru/users/rewle/sec-mkt-tokens/)
3. Привозим свои ключи на машинку (для доступа в bitbucket.y-t)
4. Работает только на ubuntu 16.0.4 с python 3.5.2 - 3.5.6, для которого есть алиас в /usr/bin/python
5. git fetch --tags
6. Правим версию докера в utils/salt/pillar/<env>/common/init.sls
7. Коммитим в мастер версию докера
8. Заполняем release.sh файл (см example и https://github.yandex-team.ru/YandexCloud/release)
9. Скачиваем bootstrap.sh
10. Выполняем bootstrap.sh

** Возможно придётся фиксить хостнеймы на машинах руками (поставить их из ростера) и добавить в /etc/hosts на 127.0.0.1
11. Довозим в соломон ip адреса
12. Заполняем строку в Releases на https://wiki.yandex-team.ru/cloud/devel/Marketplace/releases/
