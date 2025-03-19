* скачиваем fixture_permissions.yaml из артефактов последней успешной сборки master [тут](https://teamcity.yandex-team.ru/buildConfiguration/Cloud_CloudGo_IamCompileRoleFixtures?branch=%3Cdefault%3E&mode=builds). Прямая ссылка на [скачивание](https://teamcity.yandex-team.ru/repository/download/Cloud_CloudGo_IamCompileRoleFixtures/.lastFinished/fixture_permissions.yaml).
* Кладем fixture_permissions.yaml в src/main/resources/yandex/cloud/iam
* собираем jar `ya make -r` предварительно убрав из ya.make секцию `EXTERNAL_JAR` и раскомментировав `JAVA_SRCS`
* заливаем свежесобранный jar в sandbox `ya upload --ttl=inf -T=JAVA_LIBRARY -a=linux ./yandex-cloud-iam-fixtures.jar`
* заменяем id sandbox ресурса на свежий в секции `EXTERNAL_JAR`

# ИЛИ

Запускаем `./update.sh`

