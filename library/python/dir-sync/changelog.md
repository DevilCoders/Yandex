0.54
----
 * WIKI-13787: обработать ответ service_is_not_enabled

[elisei](http://staff/elisei) 2020-05-19 12:52:52+03:00

0.53
----

* [neofelis](http://staff/neofelis)

 * WIKI-13406: Releasing 0.53 of dir-sync

 * WIKI-13406: Добавил новые типы сигналов, поправил путь к документации

Добавил типы сигналов о добавлении и снятии админских полномочий с сотрудника - security_user_grant_organization_admin и security_user_revoke_organization_admin. Нужно использовать их, так как апдейты по служебной группе organization_admin не прилетают (WIKI-13406) и она будет всегда пустой.

Кроме того, сигналы о прочих модификациях групп вместо group_membership_changed теперь конкретные - user_group_added, user_group_deleted. Поправил это место чтобы не вводить в заблуждение  [ https://a.yandex-team.ru/arc/commit/6688367 ]

* [elisei](http://staff/elisei)

 * setup file for version 0.52  [ https://a.yandex-team.ru/arc/commit/6664612 ]

[neofelis](http://staff/neofelis) 2020-04-22 19:46:21+03:00

0.52
----
 * WIKI-13698: добавил обработку удаления организации в Коннекте  [ https://a.yandex-team.ru/arc/commit/6664092 ]

[elisei](http://staff/elisei) 2020-04-14 19:25:22+03:00

0.51
----
 * WIKI-13299: добавил метод добавления пользователя в команду Коннекта

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2020-03-23 18:00:00+03:00

0.50
----
 * Вернул обратно временно снятый лок с таски sync_dir_data_changes

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2019-10-03 12:00:00+03:00


0.49
----
 * Не вызывать лишний раз таски обновления для заблокированных организаций

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2019-10-02 11:15:00+03:00

0.48
----
 * Поправил вызов таски синхронизации данных организации

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2019-10-02 08:40:00+03:00

0.47
----
 * Удалил дебаг лог

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2019-10-02 02:20:00+03:00

0.46
----
 * WIKI-12537: одновременно синхронизировать данные нескольких организаций

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2019-10-02 00:55:00+03:00

0.45
----
 * WIKI-12532: не обрабатывать обновления организаций, заблокированных в Директории

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2019-09-30 14:35:00+03:00

0.42
----
 * обновил версии библиотек: celery, requests, ylock и tvm2
 * Добавил таске sync_new_organizations параметры ignore_result и time_limit

0.41
----
 * WIKI-11738: Добавил ignore_result=True для таски create_new_organization

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2019-04-15 11:50:00+03:00

0.40
----
 * WIKI-11431: Настройка DIRSYNC_DIR_TVM2_CLIENTS для unstable  [ https://github.yandex-team.ru/tools/dir-sync/commit/4e2a7bb ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-10-16 19:52:13+03:00

0.39
----
 * WIKI-11296: увеличил таймаут и добавил retry для post_service_ready  [ https://github.yandex-team.ru/tools/dir-sync/commit/5be9387 ]

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2018-09-26 14:24:59+03:00

0.38
----
 * DOGMA-610 Правка получения tvm client_id

[Vladimir Koljasinskij](http://staff/smosker@yandex-team.ru) 2018-08-28 09:42:42+03:00

0.37
----
 * DOGMA-573 Правка client_id директории

[Vladimir Koljasinskij](http://staff/smosker@yandex-team.ru) 2018-08-21 11:25:42+03:00

0.36
----
 * DOGMA-573 Работа над поддержкой TVM2 при запросах в директорию [ https://github.yandex-team.ru/tools/dir-sync/commit/2d85fb2 ]

[Vladimir Koljasinskij](http://staff/smosker@yandex-team.ru) 2018-08-14 14:30:42+03:00

0.35
----
 * WIKI-11121: чиню сбор статистики с failed status  [ https://github.yandex-team.ru/tools/dir-sync/commit/d1ef6ae ]

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2018-08-09 12:34:42+03:00

0.34
----
 * WIKI-10744: поправил проверку c object (#33)  [ https://github.yandex-team.ru/tools/dir-sync/commit/1fc216d ]

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2018-03-20 16:10:04+03:00

0.33
----
 * WIKI-10744: добавил в object недостающий атрибут id (#32)  [ https://github.yandex-team.ru/tools/dir-sync/commit/e4d6c10 ]

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2018-03-20 12:14:43+03:00

0.32
----
 * WIKI-10744: добавил обработку кода ошибки service_is_not_enabled (#28)               [ https://github.yandex-team.ru/tools/dir-sync/commit/605a5f6 ]
 * WIKI-10744: увеличил connection timeout (#29)                                        [ https://github.yandex-team.ru/tools/dir-sync/commit/a70a3a7 ]
 * WIKI-10744: костыль для случая object=None в событии group_membership_changed (#31)  [ https://github.yandex-team.ru/tools/dir-sync/commit/5c80bd5 ]

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2018-03-19 16:42:00+03:00

0.31
----
 * WIKI-10801: не обрабатываем чужие события service_enabled и service_disabled (#30)  [ https://github.yandex-team.ru/tools/dir-sync/commit/11e0e6b ]

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2018-03-15 18:06:38+03:00

0.30
----

* [Alexey Mikerin](http://staff/elisei@yandex-team.ru)

 * Поправил опечатку в названии события

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2018-03-01 23:14:40

0.29
----

* [Alexey Mikerin](http://staff/elisei@yandex-team.ru)

 * Добавил костыль для обработки событий service\_enabled и service\_disabled

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2018-03-01 20:16:31

0.28
----
 * WIKI-10742: добавил дефолтное значение в миграцию  [ https://github.yandex-team.ru/tools/dir-sync/commit/7b0f2bf ]

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2018-02-28 11:49:46+03:00

0.27
----
 * Избавляюсь от зацикливания в импортах  [ https://github.yandex-team.ru/tools/dir-sync/commit/b732b31 ]

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2018-02-27 20:48:22+03:00

0.26
----
 * Поправил импорт  [ https://github.yandex-team.ru/tools/dir-sync/commit/99c8d1d ]

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2018-02-27 18:54:36+03:00

0.25
----
 * Поправил опечатку  [ https://github.yandex-team.ru/tools/dir-sync/commit/c9db477 ]

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2018-02-27 18:30:50+03:00

0.24
----
 * WIKI-10485: проверить доступ пользователя к сервису (#26)             [ https://github.yandex-team.ru/tools/dir-sync/commit/6edb7f8 ]
 * WIKI-10742: обрабатываем события service_enabled и service_disabled (#27)  [ https://github.yandex-team.ru/tools/dir-sync/commit/f4274c5 ]

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2018-02-27 18:05:41+03:00

0.23
----
 * FORMS-1898: Публичные формы (#25)  [ https://github.yandex-team.ru/tools/dir-sync/commit/12bf845 ]

[Kirill Dunaev](http://staff/kdunaev@yandex-team.ru) 2018-02-06 14:22:32+03:00

0.22
----

* [Alexey Mikerin](http://staff/elisei@yandex-team.ru)

 * WIKI-10608: костыль для обработки устревшего формата событий  [ https://github.yandex-team.ru/tools/dir-sync/commit/46505bc ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-02-01 17:53:36+03:00

0.21
----
 * WIKI-10490: забыл переключить версию API в URL  [ https://github.yandex-team.ru/tools/dir-sync/commit/a10c0b5 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-02-01 15:25:05+03:00

0.20
----
 * WIKI-10715: переключаем на jsonfield==2.0.2  [ https://github.yandex-team.ru/tools/dir-sync/commit/4651e4b ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-01-31 15:45:18+03:00

0.19
----

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * WIKI-10490: Перейти на APIv7 Директории                                         [ https://github.yandex-team.ru/tools/dir-sync/commit/d2150f4 ]
 * WIKI-10161: Транзакционно обновлять статистику синхронизации в таске sync_data  [ https://github.yandex-team.ru/tools/dir-sync/commit/2f3b8f6 ]

* [Alexey Mikerin](http://staff/elisei@yandex-team.ru)

 * WIKI-10605: оптимизация перебора отсутствующих организаций  [ https://github.yandex-team.ru/tools/dir-sync/commit/d9d82ad ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-01-29 19:55:56+03:00

0.18
----
 * Используем JsonField с возможностью фолбека в текстовое поле (#18)  [ https://github.yandex-team.ru/tools/dir-sync/commit/30e0074 ]

[Kirill Dunaev](http://staff/kdunaev@yandex-team.ru) 2017-12-06 17:58:45+03:00

0.17
----
 * WIKI-10167: удаляем celery app из кода  [ https://github.yandex-team.ru/tools/dir-sync/commit/4e6730a ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2017-10-13 16:15:12+03:00

0.16
----
 * FORMS-1478 переложил фикстуры внутрь  [ https://github.yandex-team.ru/tools/dir-sync/commit/df214ce ]

[Yuri Nediuzhin](http://staff/yurgis@yandex-team.ru) 2017-09-25 02:19:29+03:00

0.15
----
 * FORMS-1478 ещё попытка принести с собой фикстуру  [ https://github.yandex-team.ru/tools/dir-sync/commit/556bfc2 ]

[Yuri Nediuzhin](http://staff/yurgis@yandex-team.ru) 2017-09-25 02:02:02+03:00

0.14
----
 * FORMS-1478 совместимость с не-постгресом для прохождения тестов  [ https://github.yandex-team.ru/tools/dir-sync/commit/b5cfb98 ]
 * FORMS-1478 включение фикстуры в пакет                            [ https://github.yandex-team.ru/tools/dir-sync/commit/040f44f ]

[Yuri Nediuzhin](http://staff/yurgis@yandex-team.ru) 2017-09-25 01:00:01+03:00

0.13
----
 * FORMS-1394 фикстура с дефолтным OperatingMode для тестов  [ https://github.yandex-team.ru/tools/dir-sync/commit/b74a498 ]

[Yuri Nediuzhin](http://staff/yurgis@yandex-team.ru) 2017-09-19 18:38:00+03:00

0.12
----

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * WIKI-10399: явно указываем имена celery задачам (#14)  [ https://github.yandex-team.ru/tools/dir-sync/commit/65b7070 ]

* [Yuri Nediuzhin](http://staff/yurgis@yandex-team.ru)

 * releasing version 0.11  [ https://github.yandex-team.ru/tools/dir-sync/commit/66b39c1 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2017-09-19 14:38:22+03:00

0.11
----
 * добавил в org_ctx шоткат get_org_dir_id  [ https://github.yandex-team.ru/tools/dir-sync/commit/31b63fe ]
 * little optimization                      [ https://github.yandex-team.ru/tools/dir-sync/commit/5920264 ]

[Yuri Nediuzhin](http://staff/yurgis@yandex-team.ru) 2017-09-18 14:01:48+03:00

0.10
----
 * Админка  [ https://github.yandex-team.ru/tools/dir-sync/commit/af5e26a ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2017-09-01 19:22:58+03:00

0.9
---
 * WIKI-10336: возвращать копии диктов с настройками  [ https://github.yandex-team.ru/tools/dir-sync/commit/92ddf67 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2017-08-31 18:39:53+03:00

0.8
---

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * WIKI-10336: Автоматически создаем OrgStatistics при создании Organization                          [ https://github.yandex-team.ru/tools/dir-sync/commit/fd4e136 ]
 * Починил миграцию 0006_remove_org_label_index (перед drop index надо было сделать drop constraint)  [ https://github.yandex-team.ru/tools/dir-sync/commit/c354ac8 ]

* [Alexey Mikerin](http://staff/elisei@yandex-team.ru)

 * WIKI-10239: Обрабатываем изменения тарифного плана организации (#9)  [ https://github.yandex-team.ru/tools/dir-sync/commit/838a923 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2017-08-23 14:13:10+03:00

0.7
---
 * WIKI-10296: добавляем в схему данных изменения для биллинга. (#7)

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2017-08-16 20:42:50+03:00

0.6
---
 * WIKI-10196: Директория переименовала событие по смене мастер-домена  [ https://github.yandex-team.ru/tools/dir-sync/commit/d8fa8ef ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2017-08-16 16:57:54+03:00

0.5
---

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * [OK] WIKI-10046: удаляем индекс поля Organization.label, добавляем сортиро… (#6)  [ https://github.yandex-team.ru/tools/dir-sync/commit/f2bb39c ]

* [Alexey Mikerin](http://staff/elisei@yandex-team.ru)

 * Изменил лог статус с err на warn для сообщения 'There is no signal for event ...', так как из ручки events к нам прилетают все события, а не только те, на которые мы подписаны  [ https://github.yandex-team.ru/tools/dir-sync/commit/2042186 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2017-08-14 13:43:00+03:00

0.4
---
 * WIKI-10152: не будем падать, если прилетел сигнал вне подписки.

[Alexey Mikerin](http://staff/elisei@yandex-team.ru) 2017-08-03 17:04:52+03:00

0.3
---
 * FORMS-1343 убрал unique на 1000 символов из миграций

[Yuri Nediuzhin](http://staff/yurgis@yandex-team.ru) 2017-08-01 15:10:51+03:00

0.2
---
 * FORMS-1343 убираю unique с 1000 символов из-за mysql
 * WIKI-10152 доработки по вынесению кода

[Yuri Nediuzhin](http://staff/yurgis@yandex-team.ru) 2017-07-31 21:17:51+03:00

0.1
---
initial

[Yuri Nediuzhin](http://staff/yurgis@yandex-team.ru) 2017-07-17 17:09:56+03:00

