* скачиваем fixture_permissions.yaml из артефактов посоедней успешной сборки [тут](https://teamcity.yandex-team.ru/buildConfiguration/Cloud_CloudGo_IamCompileRoleFixtures?branch=%3Cdefault%3E&mode=builds)
* заливаем в sandbox ```ya upload --ttl=inf fixture_permissions.yaml```
* обновляем resource_id в ya.make

Этут кусок чуть позже будет автоматизирован на схему с AUTOUPDATED.

