# Простые тулзы

## deploy_run.sh
Выполнение команд на каждом хосте из stage-а Deploy.

Пример:
```bash
deploy_run.sh ci-tms-stable 'cd /logs && (nohup gzip -f ci-tmsTemporal-2022-*.log) &'
```
Выполняет сжатие логов для [ci-tms-table](https://deploy.yandex-team.ru/stages/ci-tms-stable) в Deploy Unit-е `tms` и Box-е `tms` (список всех stage-ей задается в файле `_stages.sh`).

Для этого примера можно передать флаг в переменные окружения: `export SSH_FLAGS="-f"`. Такой флаг заставит команду SSH выполняться без ожидания результата (т.е. операции действительно будут выполнены в фоне).

## deploy_scp.sh

Заливает файл или файлы в корень root пользователя на каждом хосте из stage-а Deploy.

Пример:
```bash
deploy_scp.sh ci-tms-stable ci-core.jar
```

Заливает локальный файл `ci-core.jar` в `~/ci-core.jar` для [ci-tms-table](https://deploy.yandex-team.ru/stages/ci-tms-stable) в Deploy Unit-е `tms` и Box-е `tms` (аналогично предыдущему пункту).

После чего файл можно установить на всех хостах, с паузой:
```bash
deploy_run.sh ci-tms-stable 'mv /ci-tms/lib/ci-core.jar /ci-tms/lib/ci-core.ja_ && mv ~/ci-core.jar /ci-tms/lib/ && killall java && sleep 30'
```
