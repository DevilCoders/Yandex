0.14
----

* [Nikita Sukhorukov](http://staff/cracker@yandex-team.ru)

 * COMMONPY-210: add arc support (#17)  [ https://github.yandex-team.ru/tools/gitchronicler/commit/b761114 ]

[cracker](http://staff/cracker@yandex-team.ru) 2019-09-16 18:19:56+03:00

0.13
----

 * Do not mangle up the changelog entry text with overzealous strip() [ https://github.yandex-team.ru/tools/gitchronicler/commit/91c58c4 ]

[Vladimir Koljasinskij](http://staff/smosker@yandex-team.ru) 2019-04-09 18:49:08+00:00

0.12
----

* [Ivan Moskvin](http://staff/ivan.moskvin@gmail.com)

 * Add versioning scheme for next version generation  [ https://github.yandex-team.ru/tools/gitchronicler/commit/b245328 ]

[Alex Koshelev](http://staff/alexkoshelev@yandex-team.ru) 2019-03-19 13:37:08+00:00

0.11
----

* [Anton Vasilyev](http://staff/hhell@yandex-team.ru)

 * py3 fix  [ https://github.yandex-team.ru/tools/gitchronicler/commit/e3c9572 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-07-12 16:26:21+03:00

0.10
----
 * Заменил tzlocal на dateutil  [ https://github.yandex-team.ru/tools/gitchronicler/commit/cce3655 ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2017-04-18 13:54:19+03:00

0.9
---
 * Удалил дебианизацию, собираем релизером.  [ https://github.yandex-team.ru/tools/gitchronicler/commit/47210f1 ]
 * Fix git remote detection                  [ https://github.yandex-team.ru/tools/gitchronicler/commit/0e557e8 ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2017-04-17 15:12:05+03:00

0.8
---

  * Возвращать юникод из метода get_changelog_records()

 [Oleg Gulyaev](https://staff.yandex-team.ru/o-gulyaev@yandex-team.ru) 2017-03-20 14:58:00

0.7
---

  * Метод, возвращающий пары (версия, запись в changelog

 [Oleg Gulyaev](https://staff.yandex-team.ru/o-gulyaev@yandex-team.ru) 2017-03-20 14:08:00

0.6
---

  * Fix git log when no from\_version

 [Kirill Sibirev](https://staff.yandex-team.ru/sibirev@yandex-team.ru) 2016-12-02 16:23:33

0.5
---

  * Переложил все в GitCtl на уровень модуля
  * Минимальный эскепинг md. Fix #7
  * Записываем таймстемп с таймзоной
  * Получаем hub\_url из гитконфига
  * Упростил структуру файлов

 [Kirill Sibirev](https://staff.yandex-team.ru/sibirev@yandex-team.ru) 2016-12-02 13:57:20

0.4
---

 * [Aleksey Ostapenko](https://staff.yandex-team.ru/Aleksey%20Ostapenko)

  * Get default editor from $VISUAL or $EDITOR

 [Kirill Sibirev](https://staff.yandex-team.ru/sibirev@yandex-team.ru) 2016-11-21 12:33:23

0.3
---

 * [Oleg Gulyaev](https://staff.yandex-team.ru/Oleg%20Gulyaev)

  * В конце commit message добавляются двойные кавычки
  * Escape markdown syntax in commit messages

 * [Артём Пендюрин](https://staff.yandex-team.ru/Артём%20Пендюрин)

  * Добавлена возможность задать номер новой версии в вызов write

 [Oleg Gulyaev](https://staff.yandex-team.ru/o-gulyaev@yandex-team.ru) 2016-08-11 17:06:00

0.2
---

  * write() returns 'None' if there was no changelog file

 [Oleg Gulyaev](https://staff.yandex-team.ru/o-gulyaev@yandex-team.ru) 2016-07-06 19:17:00

0.1
---

 * [Dmitry Prokofyev](https://staff.yandex-team.ru/Dmitry%20Prokofyev)

  * initial

 [Oleg Gulyaev](https://staff.yandex-team.ru/o-gulyaev@yandex-team.ru) 2016-07-06 17:00:00

