# HOWTO: Build patched repository-s3 plugin

Зависимости
- git
- docker или jdk 16+

Клонируем исходники c патчем c из https://github.yandex-team.ru/mdb/elasticsearch/

	git clone git@github.yandex-team.ru:mdb/elasticsearch.git

Опционально, синхронизируем с репозиторием эластика (нас интересуют только протегированные коммиты)

	git remote add elastic git@github.com:elastic/elasticsearch.git
	git fetch elastic
	git push --tags

Находим нужный релиз и применяем к нему патч, коммит патча можно найти по тегу для предыдущего релиза `patched-7.10.1`

	git checkout v7.10.2
	git cherry-pick patched-7.10.1

Собираем плагин

    cd plugins/repository-s3/
    ./build_docker.sh <elasticsearch git root full path>

Если все ок, то тегаем и коммитим.

	git tag patched-7.10.2
	git push --tags

Копируем собранный плагин в нужное вам место

	cp build/distributions/repository-s3-7.10.2.zip <path>

