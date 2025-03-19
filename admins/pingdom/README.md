# Pingdom cli

![](https://jing.yandex-team.ru/files/coldmind/28816007-2d303ec8-769b-11e7-8d75-8d1fbd93592c.png =100x)


Используется для работы с чеками. Основной сценарий использования: пауз пачки чеков во время крупного факапа, с целью экономии средств на отправке SMS членам команды.

#### Получить бинарь


Бинарник под mac: `wget -q https://proxy.sandbox.yandex-team.ru/1539561368 -O pingdom_cli && chmod +x pingdom_cli && ./pingdom_cli --help`



Бинарник под linux: `wget -q https://proxy.sandbox.yandex-team.ru/1539568236 -O pingdom_cli && chmod +x pingdom_cli && ./pingdom_cli --help`

![asfd](https://jing.yandex-team.ru/files/coldmind/vosklznak.png =9x) Для корректной работы требуется API токен пингдома в переменной среды `PINGDOM_APIKEY`.

При первом запуске бинарника `./pingdom_cli`, без `PINGDOM_APIKEY`, выведется ссылка на YAV, с нужным имеменм ключа, в нём и лежит ключ.

### Инструкция
Действия, которые можно совершать с чеками:
* list
* pause
* unpause


Механизмы фильтрации чеков:
* --all
* --by-name[s]
* --by-tag[s]
* --by-status

![cli_args](https://jing.yandex-team.ru/files/coldmind/Untitled%20Diagram.png)

*Рис. 1. Варианты комбинирования аргументов командной строки*

---
### Примеры использования
---
 Посмотрим все чеки, которые имеем.

 Примечание: чеки будут выводиться в отсортированном по статусу порядке, для удобства восприятия.
 Порядок: up, unknown, paused, down.
```
./pingdom_cli list --all
```
Результат:
![all_checks](https://jing.yandex-team.ru/files/coldmind/2020-06-01-192831_1420x1872_scrot.png)


---

Посмотрим чеки, которые сейчас на паузе
```
./pingdom_cli list --by-status paused
```
![paused_checks](https://jing.yandex-team.ru/files/coldmind/2020-06-01-193619_1062x346_scrot.png)

---

Посмотрим чеки, принадлежащие двум тэгам: `drm`, и `ott`.

Примечание: можно использовать при перечислении как пробел, так и запятые. А так же как ключ `--by-tag`, так и `--by-tags`.
```
./pingdom_cli list --by-tags drm ott
./pingdom_cli list --by-tag drm,ott
./pingdom_cli list --by-tag kp-frontend
```
![drm_ott](https://jing.yandex-team.ru/files/coldmind/2020-06-01-193806_1050x629_scrot.png)

---

Поставим на паузу чеки из тэга `drm`:

```
./pingdom_cli pause --by-tags drm
```
![](https://jing.yandex-team.ru/files/coldmind/2020-06-01-194447_1021x387_scrot.png)


---
Снимим с паузы чеки из тэга `drm`:

```
./pingdom_cli unpause --by-tags drm
```
![](https://jing.yandex-team.ru/files/coldmind/2020-06-01-194511_1007x388_scrot.png)


---



***Бэйджик под названием ненастоящий и сделан исключительно ради понтов.***
