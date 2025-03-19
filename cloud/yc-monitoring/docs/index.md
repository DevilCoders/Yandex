Здесь живет документация по алертам из [yc-monitoring](https://bb.yandex-team.ru/projects/CLOUD/repos/yc-monitoring/browse/docs).

Править нужно Pull-Request-ом в bitbucket. Работает авто-синхронизация в Аркадию и авто-деплой на docs.yandex-team.ru.

Чтобы добавить страницу нового алерта:
1. Создаем файл `<TEAM>/alert-name.md`. Копируем из `_template.md` и правим.
2. Добавляем пункт в меню: `toc.yaml`.
3. Делаем PR, мержим.
4. Ждем некоторое время.

_Не забываем проставить ссылку на страницу алерта в сам Juggler и Solomon-алерт._

Протестировать внешний вид локально можно так:
- монтируем Аркадию,
- копируем содержимое `docs` локально в каталог `cloud/yc-monitoring/docs` Аркадии,
- запускаем `ya make` в каталоге `cloud/yc-monitoring/docs` Аркадии,
- распаковываем получившийся архив: `tar xvf cloud-yc-monitoring-docs.tar.gz`,
- открываем `index.html` браузером.
