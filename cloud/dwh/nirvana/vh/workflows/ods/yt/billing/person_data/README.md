#### Описание

Данные физических и юридических лиц зарегистрированных в биллинге

Доступ к данным ограничен - нужно запросить роль "Персональные данные" [тут](https://abc.yandex-team.ru/services/yc-dwh-pii-read/)

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/person_data)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/person_data)

* `billing_account_id` - идентификатор [платежного аккаунта](../billing_accounts/README.md)
* `type` - тип пользователя: физическое (individual) или юридическое лицо (company)
* `original_type` - тип пользователя, содержит информацию о стране
* `name` - имя и фамилия пользователя для физических лиц / имя компании для юридических
* `person_data` - Информация про физ/юр. лицо: ИНН, БИК, адрес и тд.
