# Трафик

Антиробот может анализировать запросы, только если трафик сервиса проходит через него.

Процедура направления трафика различается в зависимости от того, что использует сервис для маршрутизации запросов.

#### Балансер
Обратитесь к администраторам балансера с просьбой направить трафик в Антиробот.
#### Nginx
Используйте специальный модуль. Описание модуля и работы с ним см. по ссылке: [https://github.yandex-team.ru/ezaitov/nginx-yandex-antirobot-module/blob/master/README.md](https://github.yandex-team.ru/ezaitov/nginx-yandex-antirobot-module/blob/master/README.md).
#### Другой сервер
Если ваш сервис использует другой сервер для маршрутизации пользовательских запросов (не балансер или nginx), то задачу направления трафика в Антиробот вы решаете самостоятельно. Универсальных инструкций или модулей для таких случаев не разработано.

