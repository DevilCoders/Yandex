0.78
----

* BALANCERSUPPORT-2949: YP_ENDPOINT_SETS to YP_ENDPOINT_SETS_SD

[smosker](http://staff/smosker) 2021-07-23 15:12:21+03:00

0.77
----

* Releasing hot-fix

[denis-p](http://staff/denis-p) 2021-07-01 16:08:19+03:00

0.76
----

* [exprmntr](http://staff/exprmntr)

 * DEVTOOLS-7781 - remove NO_LINT()

DEVTOOLS-7781  [ https://a.yandex-team.ru/arc/commit/8098110 ]

* [hhell](http://staff/hhell)

 * YPSUPPORT-420: releaser hosts/ssh/pssh for ydeploy  [ https://a.yandex-team.ru/arc/commit/8029233 ]

* [alexeykruglov](http://staff/alexeykruglov)

 * DEVRULES-147: set licenses for local contribs

Проставляю лицензии локальным контрибам или тем местам, где обнаружил упоминание плохих лицензий.  [ https://a.yandex-team.ru/arc/commit/7979979 ]

* [vmazaev](http://staff/vmazaev)

 * CROWDFUNDING-3: fix parameters passing after click update

Замечание: https://st.yandex-team.ru/CROWDFUNDING-3#604b1793437ec60bb9f800a8  [ https://a.yandex-team.ru/arc/commit/7942612 ]

[smosker](http://staff/smosker) 2021-06-23 15:12:21+03:00

0.75
----
 * Правка команды создания стенда  [ https://a.yandex-team.ru/arc/commit/7831148 ]

[smosker](http://staff/smosker) 2021-02-04 20:58:47+03:00

0.74
----
 * COMMONPY-290 Поддержка создания драфтов в деплое  [ https://a.yandex-team.ru/arc/commit/7819684 ]
 * COMMONPY-289 добавлен комментарий для деплоя      [ https://a.yandex-team.ru/arc/commit/7818849 ]

[smosker](http://staff/smosker) 2021-02-01 17:32:28+03:00

0.73
----
 * COMMONPY-288 правка команды add_logs  [ https://a.yandex-team.ru/arc/commit/7764202 ]

[smosker](http://staff/smosker) 2021-01-19 20:53:10+03:00

0.72
----
 * COMMONPY-285 Правка включения пушклиента  [ https://a.yandex-team.ru/arc/commit/7539616 ]

[smosker](http://staff/smosker) 2020-11-02 13:45:50+03:00

0.71
----
 * COMMONPY-282 Правка настройки доставки логов для балансера  [ https://a.yandex-team.ru/arc/commit/7482592 ]

[smosker](http://staff/smosker) 2020-10-21 12:24:31+03:00

0.70
----

* [neofelis](http://staff/neofelis)

 * Добавляю возможность указывать у каждого deploy_box его имя

```
deploy_units = deploy_unit:box,deploy_unit_2:box_2 ...
```

PR from branch users/neofelis/feature/multiplebox  [ https://a.yandex-team.ru/arc/commit/7407661 ]

[cracker](http://staff/cracker) 2020-09-30 12:19:23+03:00

0.69
----
 * update readme                                                                      [ https://a.yandex-team.ru/arc/commit/7341663 ]
 * COMMONPY-280: [releaser] Поддержать добавление push client на инстансы балансеров  [ https://a.yandex-team.ru/arc/commit/7330466 ]

[cracker](http://staff/cracker) 2020-09-18 14:25:21+03:00

0.68
----
 * COMMONPY-270: [releaser] Выкатка стендов в Deploy+awacs  [ https://a.yandex-team.ru/arc/commit/7308834 ]

[cracker](http://staff/cracker) 2020-09-07 23:37:08+03:00

0.67
----
 * Support --direct-push for arc for e.g. release branches  [ https://a.yandex-team.ru/arc/commit/6981128 ]

[hhell](http://staff/hhell) 2020-06-17 16:01:07+03:00

0.66
----
 * Add box option to release workflows  [ https://a.yandex-team.ru/arc/commit/6920083 ]

[cracker](http://staff/cracker) 2020-06-10 10:28:03+03:00

0.65
----

* [denis-p](http://staff/denis-p)

 * COMMONPY-267: Initial support for Y.Deploy  [ https://a.yandex-team.ru/arc/commit/6918467 ]

* [cracker](http://staff/cracker)

 * Change .devexp.json

[denis-p](http://staff/denis-p) 2020-06-09 18:28:58+03:00

0.64
----

* [cracker](http://staff/cracker)

 * Вернул параметр version в changelog [ https://a.yandex-team.ru/arc/commit/6786134 ]

* [smosker](http://staff/smosker)

 * Поддержка использования на виртуалках [ https://a.yandex-team.ru/arc/commit/6755253 ]

[cracker](http://staff/cracker) 2020-05-14 18:45:43+03:00

0.63
----

* [cracker](http://staff/cracker)

 * COMMONPY-253: Внести releaser в ya tool                                                                                                  [ https://a.yandex-team.ru/arc/commit/6590247 ]
 * COMMONPY-251: Добавить возможность указывать в настройках target_state                                                                   [ https://a.yandex-team.ru/arc/commit/6530329 ]

[cracker](http://staff/cracker) 2020-04-07 15:44:10+03:00

0.62
----
 * Правка импорта команд для поддержки совместимости
[smosker](http://staff/smosker@yandex-team.ru) 2020-03-11 18:31:58+03:00

0.61
----

* [Kirill Kartashov](http://staff/qazaq@yandex-team.ru)

 * Fixed choosing of VCS  [ https://github.yandex-team.ru/tools/releaser/commit/5a370be ]

* [Dmitry Orlov](http://staff/feakuru@yandex-team.ru)

 * Added note about zsh screening to REAME  [ https://github.yandex-team.ru/tools/releaser/commit/6a8da21 ]

[Kirill Kartashov](http://staff/qazaq@yandex-team.ru) 2020-01-20 15:23:47+03:00

0.60
----

* [Nikita Sukhorukov](http://staff/cracker@yandex-team.ru)

 * COMMONPY-244: remove hardcoded docker repository (#139)  [ https://github.yandex-team.ru/tools/releaser/commit/cd7c487 ]

[cracker](http://staff/cracker@yandex-team.ru) 2019-12-23 13:26:53+03:00

0.59
----
 * COMMONPY-241 Делаем pull в транке до получения нужной версии в Арке[ https://github.yandex-team.ru/tools/releaser/commit/040f0bbad ]

[smosker](http://staff/smosker@yandex-team.ru) 2019-12-17 11:14:58+03:00

0.58
----
 * COMMONPY-228 fix getting new changes [ https://github.yandex-team.ru/tools/releaser/commit/74b817 ]

[smosker](http://staff/smosker@yandex-team.ru) 2019-10-09 17:00:58+03:00

0.57
----
 * fix stand workflow  [ https://github.yandex-team.ru/tools/releaser/commit/e56f016 ]

[cracker](http://staff/cracker@yandex-team.ru) 2019-09-19 17:55:58+03:00

0.56
----

* [Nikita Sukhorukov](http://staff/cracker@yandex-team.ru)

 * COMMONPY-210: arc support (#136)  [ https://github.yandex-team.ru/tools/releaser/commit/6fb4cce ]

* [Anton Chaporgin](http://staff/chapson@yandex-team.ru)

 * Update README.md  [ https://github.yandex-team.ru/tools/releaser/commit/aebcee2 ]

* [Olga Kozlova](http://staff/olgakozlova@yandex-team.ru)

 * Поправила ссылку на run.sh в readme (#134)  [ https://github.yandex-team.ru/tools/releaser/commit/d60e839 ]

[cracker](http://staff/cracker@yandex-team.ru) 2019-09-18 22:10:07+03:00

0.55
----
 * Bump and rebuild due to an unfortunate pypi version release.

[Anton Vasilyev](http://staff/hhell@yandex-team.ru) 2019-04-16 15:27:32+03:00

0.54
----
 * Remove a kludge that did not work as intended in some situation. Closes: #131  [ https://github.yandex-team.ru/tools/releaser/commit/c323bb0 ]

[Anton Vasilyev](http://staff/hhell@yandex-team.ru) 2019-04-16 15:20:43+03:00

0.53
----
* Deploy message that involves all the versions from the current

[Vladimir Koljasinskij](http://staff/smosker@yandex-team.ru) 2019-04-11 11:19:18+03:00


0.52
----

* [Anton Vasilyev](http://staff/hhell@yandex-team.ru)

 * An attempt to use a complete workflow for releasing a new version of the releaser itself           [ https://github.yandex-team.ru/hhell/releaser/commit/782cf36 ]
 * ssh command over additional arguments alongside the --command; send some of the outputs to stderr  [ https://github.yandex-team.ru/hhell/releaser/commit/ecc9f33 ]

* [Alexander Artemenko](http://staff/art@yandex-team.ru)

 * Исправлена ошибка в workflow "release", связанная с изменившейся сигнатурой функции "changelog".  [ https://github.yandex-team.ru/hhell/releaser/commit/bdaf3da ]

[Anton Vasilyev](http://staff/hhell@yandex-team.ru) 2019-04-09 18:17:47+03:00

0.51
----
* fix param name

[Vladimir Koljasinskij](http://staff/smosker@yandex-team.ru) 2019-03-27 13:54:18+03:00


0.50
----
* Allow to set versioning schema and release type

[Vladimir Koljasinskij](http://staff/smosker@yandex-team.ru) 2019-03-26 12:39:18+03:00

0.49
----

[Anton Vasilyev](http://staff/hhell@yandex-team.ru) 2018-11-13 16:39:18+03:00

0.48
----

* [Alexander Artemenko](http://staff/art@yandex-team.ru)

 * Всё-таки используем add-domain, так как в click==7.0 иначе не работает.                                        [ https://github.yandex-team.ru/tools/releaser/commit/912fff3 ]
 * Функция parse_ticket_key исправлена так, чтобы искать id тикета по всей строке, а не как полное соответствие.  [ https://github.yandex-team.ru/tools/releaser/commit/d565bbe ]
 * Добавлена команда stand-ticket, чтобы можно было строить несколько окружений, связанных между собой.           [ https://github.yandex-team.ru/tools/releaser/commit/2fa1dad ]
 * Добавлена опция --no-get-ticket-from-last-commit-message, отключающая --get-ticket-from-last-commit-message.   [ https://github.yandex-team.ru/tools/releaser/commit/40863d5 ]
 * Команда deploy теперь в случае если файл с дампом не найден, выдаёт более корректную ошибку.                   [ https://github.yandex-team.ru/tools/releaser/commit/b979bd0 ]
 * Заменил обратно add-domain на add_domain в workflow-команде stand.                                             [ https://github.yandex-team.ru/tools/releaser/commit/1090ba9 ]
 * В команды deploy, release и stand теперь можно переопределять переменные окружения.                            [ https://github.yandex-team.ru/tools/releaser/commit/a48fdc0 ]
 * Теперь в команде deploy можно указать сколько инстансов поднять и в каких ДЦ.                                  [ https://github.yandex-team.ru/tools/releaser/commit/f6b26e5 ]

[Anton Vasilyev](http://staff/hhell@yandex-team.ru) 2018-11-13 16:37:55+03:00

0.47
----

* [Anton Vasilev](http://staff/hhell@yandex-team.ru)

 * Support click 6.x and click 7.x  [ https://github.yandex-team.ru/tools/releaser/commit/42d43ba ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-10-30 13:53:01+03:00

0.46
----

* [Anton Vasilev](http://staff/hhell@yandex-team.ru)

 * Fix the qloud _check_error in py3; fixes: #112  [ https://github.yandex-team.ru/tools/releaser/commit/2ac5b67 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-10-29 12:57:23+03:00

0.45
----
 * target_state=None для releaser stand      [ https://github.yandex-team.ru/tools/releaser/commit/45d9337 ]
 * Дополнил документацию по --target-state.  [ https://github.yandex-team.ru/tools/releaser/commit/cd4e201 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-10-29 12:53:04+03:00

0.44
----

* [Nikolay Tretyak](http://staff/ndtretyak@yandex-team.ru)

 * Возможность указать target_state при деплое  [ https://github.yandex-team.ru/tools/releaser/commit/a9c3b95 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-10-26 13:50:07+03:00

0.43
----
 * Фиктивная версия из-за неправильной заливки предыдущей на PyPi

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-10-17 11:38:50+03:00

0.42
----

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * Поправил документацию по опции --image  [ https://github.yandex-team.ru/tools/releaser/commit/c039a8b ]

* [Anton Vasilev](http://staff/hhell@yandex-team.ru)

 * 'hosts' and 'pssh' support  [ https://github.yandex-team.ru/tools/releaser/commit/58bb287 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-10-05 14:44:59+03:00

0.41
----

* [Maksim Kuznetcov](http://staff/mkznts@yandex-team.ru)

 * Обнулять размер файла с версией перед перезаписью  [ https://github.yandex-team.ru/tools/releaser/commit/ff536d1 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-09-05 12:35:52+03:00

0.40
----

* [Kirill Dunaev](http://staff/kdunaev@yandex-team.ru)

 * Запускаем команды через ssh  [ https://github.yandex-team.ru/tools/releaser/commit/769437a ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-07-30 13:20:26+03:00

0.39
----

* [Anton Vasilyev](http://staff/hhell@yandex-team.ru)

 * Updated changelog_converter for the newer deblibs  [ https://github.yandex-team.ru/tools/releaser/commit/1a28bf7 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-07-23 12:24:46+03:00

0.38
----

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * gitchronicler==0.11  [ https://github.yandex-team.ru/tools/releaser/commit/dffbe6d ]

* [Anton Vasilyev](http://staff/hhell@yandex-team.ru)

 * Update deblibs  [ https://github.yandex-team.ru/tools/releaser/commit/da64ded ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-07-12 17:07:22+03:00

0.37
----
 * setup.py v==0.37                                                                    [ https://github.yandex-team.ru/tools/releaser/commit/671d66f ]
 * Добавил недостающую опцию pull_vcs в вызов команды deploy у workflow команды stand  [ https://github.yandex-team.ru/tools/releaser/commit/a4ad304 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-07-12 13:56:50+03:00

0.36
----
 * Версия 0.36                                                                  [ https://github.yandex-team.ru/tools/releaser/commit/b9a69b0 ]
 * Поддержка опции pull-vcs для команд release, release_changelog и changelog.  [ https://github.yandex-team.ru/tools/releaser/commit/7d4a0a2 ]
 * Поддержка опции pull-vcs для команды deploy.                                 [ https://github.yandex-team.ru/tools/releaser/commit/91e3d83 ]
 * Добавлен вывод ошибок импорта модулей с командами CLI.                       [ https://github.yandex-team.ru/tools/releaser/commit/0362327 ]

[Igor Starikov](http://staff/idlesign@yandex-team.ru) 2018-06-14 12:48:24+07:00

0.35
----
 * Добавлена поддержка опции --ver.                               [ https://github.yandex-team.ru/tools/releaser/commit/fc3f0b4 ]
 * Для build, release и stand добавлена опция --pull-base-image.  [ https://github.yandex-team.ru/tools/releaser/commit/189bb4c ]

[Igor Starikov](http://staff/idlesign@yandex-team.ru) 2018-06-05 16:23:34+07:00

0.34
----

* [Kirill Sibirev](http://staff/sibirev@yandex-team.ru)

 * Update README.md  [ https://github.yandex-team.ru/hhell/releaser/commit/d25a755 ]

* [Anton Vasilyev](http://staff/hhell@yandex-team.ru)

 * Use cfg.PROJECT in the registry image url (instead of the hardcoded '/tools/')  [ https://github.yandex-team.ru/hhell/releaser/commit/382c45d ]

[Anton Vasilyev](http://staff/hhell@yandex-team.ru) 2018-05-11 11:59:17+03:00

0.33
----
 * Fix #90 Добавление деплой хука в стенды (#92)  [ https://github.yandex-team.ru/tools/releaser/commit/aa92bf5 ]
 * Команда добавления деплой хука #90             [ https://github.yandex-team.ru/tools/releaser/commit/40bdf63 ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2018-04-11 15:17:18+03:00

0.32
----

* [Kirill Sibirev](http://staff/sibirev@yandex-team.ru)

 * Добавляем домен в окружение со стендом         [ https://github.yandex-team.ru/tools/releaser/commit/b81052d ]
 * Возможность задавать путь до дампа в конфиге   [ https://github.yandex-team.ru/tools/releaser/commit/f03633c ]
 * Проверяем существующие домены перед созданием  [ https://github.yandex-team.ru/tools/releaser/commit/7cda039 ]
 * Возможность задать балансер для нового домена  [ https://github.yandex-team.ru/tools/releaser/commit/70bfa9a ]

* [Maksim Kuznetcov](http://staff/mkznts@yandex-team.ru)

 * Возвращать image_hash в unicode для поддержки Python 3  [ https://github.yandex-team.ru/tools/releaser/commit/cb0ab7d ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2018-04-05 11:46:43+03:00

0.31
----

* [Kirill Sibirev](http://staff/sibirev@yandex-team.ru)

 * Добавил пример ~/.release.hjson  [ https://github.yandex-team.ru/tools/releaser/commit/33d15ba ]

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * WIKI-10760: Не передаем "comment" в dump /upload, если комментарий деплоя не задан  [ https://github.yandex-team.ru/tools/releaser/commit/373c3f0 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-02-19 19:39:21+03:00

0.30
----
 * WIKI-10616: Передаем ключ тикета в Qloud в команде stand  [ https://github.yandex-team.ru/tools/releaser/commit/a85ebec ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2018-01-10 20:10:42+03:00

0.29
----

* [Maksim Kuznetcov](http://staff/mkznts@yandex-team.ru)

 * Команда для отображения статуса инстансов  [ https://github.yandex-team.ru/tools/releaser/commit/cbb244b ]

* [Kirill Sibirev](http://staff/sibirev@yandex-team.ru)

 * Ненулевой статус для ошибки в workflow-командах  [ https://github.yandex-team.ru/tools/releaser/commit/350fe6b ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2017-12-18 15:53:37+03:00

0.28
----
 * Correct exit codes  [ https://github.yandex-team.ru/tools/releaser/commit/0fbd9db ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2017-12-12 13:40:31+03:00

0.27
----

* [Yuriy Remnev](http://staff/remnev@yandex-team.ru)

 * Explicit import of debian_support.Version  [ https://github.yandex-team.ru/tools/releaser/commit/10faccc ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2017-06-30 17:02:30+03:00

0.26
----

* [Yuriy Remnev](http://staff/remnev@yandex-team.ru)

 * Replace print with echo  [ https://github.yandex-team.ru/tools/releaser/commit/fb84ce5 ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2017-06-30 12:33:36+03:00

0.25
----
 * Не подключаем команды, которые не импортятся  [ https://github.yandex-team.ru/tools/releaser/commit/ccdb0e8 ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2017-05-16 16:59:54+03:00

0.24
----
 * Подробности в --dry-run для deploy        [ https://github.yandex-team.ru/tools/releaser/commit/f3be85a ]
 * В дампе могут быть не-ascii символы       [ https://github.yandex-team.ru/tools/releaser/commit/9b4ebf4 ]
 * Поддержка --dry-run в env_dump            [ https://github.yandex-team.ru/tools/releaser/commit/4092f78 ]
 * \--dump — обязательная опция для env_dump [ https://github.yandex-team.ru/tools/releaser/commit/cba5d8b ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2017-05-04 18:15:21+03:00

0.23
----
 * releaser version cmd  [ https://github.yandex-team.ru/tools/releaser/commit/46d764d ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2017-04-19 15:19:43+03:00

0.22
----
 * gitchronicler при мерже оказался в двух местах  [ https://github.yandex-team.ru/tools/releaser/commit/0d59dde ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2017-04-18 14:55:59+03:00

0.21
----
 * gitchronicler==0.10              [ https://github.yandex-team.ru/tools/releaser/commit/d0d8637 ]
 * Опциональные python-зависимости  [ https://github.yandex-team.ru/tools/releaser/commit/2b980b6 ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2017-04-18 14:00:04+03:00

0.20
----

* [Kirill Sibirev](http://staff/sibirev@yandex-team.ru)

 * gitchronicler==0.9               [ https://github.yandex-team.ru/tools/releaser/commit/73374c6 ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2017-04-17 15:24:08+03:00

0.19
----
 * Мелкие правки                              [ https://github.yandex-team.ru/tools/releaser/commit/0ab681b ]
 * Доработка env_delete, добавление в README  [ https://github.yandex-team.ru/tools/releaser/commit/239bfad ]
 * Удаление окружений по wildcard             [ https://github.yandex-team.ru/tools/releaser/commit/f7ebc0e ]
 * Рефакторинг QloudClient                    [ https://github.yandex-team.ru/tools/releaser/commit/5da2902 ]
 * Команда удаления окружения                 [ https://github.yandex-team.ru/tools/releaser/commit/ebc3825 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2017-04-07 17:53:57+03:00

0.18
----
 * Обновил README; добавил забытую опцию --dump в команду stand  [ https://github.yandex-team.ru/tools/releaser/commit/5eb029b ]
 * Команда для добавления домена в окружение                     [ https://github.yandex-team.ru/tools/releaser/commit/97a35ec ]
 * Команда для дампа окружения в файл                            [ https://github.yandex-team.ru/tools/releaser/commit/5d9ee08 ]
 * Опция с файлом с дампом деплоящегося окружения                [ https://github.yandex-team.ru/tools/releaser/commit/449289e ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2017-04-06 18:47:56+03:00

0.17
----
 * Починил форматирование комментария деплоя для случая отсутствующего changelog.md  [ https://github.yandex-team.ru/tools/releaser/commit/615cd72 ]
 * Документация про --deploy-comment-format                                          [ https://github.yandex-team.ru/tools/releaser/commit/6c8f28f ]
 * Переключил версию в setup.py                                                      [ https://github.yandex-team.ru/tools/releaser/commit/086e293 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2017-03-22 15:00:29+03:00

0.16
----

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * Комментарий выкладки в Qloud  [ https://github.yandex-team.ru/tools/releaser/commit/08c31a3 ]

* [Sergey Kovalev](http://staff/remedy@yandex-team.ru)

 * Fix convert_changelog --dry-run  [ https://github.yandex-team.ru/tools/releaser/commit/dc99a88 ]

* [Marina Kamalova](http://staff/mkamalova@yandex-team.ru)

 * yandex-internal-root-ca in Dockerfile  [ https://github.yandex-team.ru/tools/releaser/commit/96a4950 ]
 * releaser inside docker dependencies    [ https://github.yandex-team.ru/tools/releaser/commit/09dc319 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2017-03-20 20:51:39+03:00

0.15
----

* [Kirill Sibirev](http://staff/sibirev@yandex-team.ru)

 * Forgotten setup.py update  [ https://github.yandex-team.ru/tools/releaser/commit/3673bde ]

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * Команда ssh  [ https://github.yandex-team.ru/tools/releaser/commit/d6d5c52 ]

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2017-01-20 14:33:56+04:00

0.14
----
 * Add requests[security] for SNI support  [ https://github.yandex-team.ru/tools/releaser/commit/86528fe ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2017-01-10 14:12:51+03:00

0.13
----
 * Более гибкая конфигурируемость                           [ https://github.yandex-team.ru/tools/releaser/commit/de6abf0 ]
 * Fix #46 — скачиваем внутренний сертификат, если его нет  [ https://github.yandex-team.ru/tools/releaser/commit/f0d9430 ]
 * Удалил неиспользуемую больше опцию                       [ https://github.yandex-team.ru/tools/releaser/commit/9fe03f2 ]
 * run_with_output всегда выводит испольняемую команду      [ https://github.yandex-team.ru/tools/releaser/commit/1ae63a8 ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2017-01-09 19:40:48+03:00

0.12
----

* [Kirill Sibirev](http://staff/sibirev@yandex-team.ru)

 * chronickler==0.6 + доработки воркфлоу  [ https://github.yandex-team.ru/tools/releaser/commit/7b84bc7 ]
 * version 0.11 in setup.py               [ https://github.yandex-team.ru/tools/releaser/commit/7e54181 ]

* [Alex Koshelev](http://staff/alexkoshelev@yandex-team.ru)

 * Использование самого docker для дистрибуции releaser  [ https://github.yandex-team.ru/tools/releaser/commit/fe9aec4 ]

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2016-12-29 16:59:18+03:00

0.11
----

* [Kirill Sibirev](http://staff/sibirev@yandex-team.ru)

 * Подавляем ворнинги urllib
 * verify=False для запроса в dockerinfo

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * Fix README\.md

[Kirill Sibirev](http://staff/sibirev@yandex-team.ru) 2016-12-28 17:27:01

0.10
----

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * Опция выбора инстанса Qloud int/ext
 * Добавить флаг для проброса версии приложения в Dockerfile
 * Qloud теперь использует sha\-digest у компонентов вместо hash

* [Nikita Zubkov](http://staff/zubchick@yandex-team.ru)

 * Add new cli flag for specifying qloud project

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2016-12-27 15:26:35

0.9
---

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * Убираю аргумент в docker build для сброса кэша, он ломает сборки его не использующие

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2016-12-01 13:37:25

0.8
---

* [Kirill Sibirev](http://staff/sibirev@yandex-team.ru)

 * Разбил большой файл с командами на модули

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * Аргумент в docker build для сброса Docker кэша

* [Anton Chaporgin](http://staff/chapson@yandex-team.ru)

 * Update README\.md

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2016-11-29 16:21:20

0.7
---

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * Исправил ошибку в команде deploy для случая нескольких приложений

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2016-11-08 14:15:00

0.6
---

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * Переименовал --no-commit в --no-git-commit, --no-push в --no-git-push

* [Aleksey Ostapenko](http://staff/kbakba@yandex-team.ru)

 * --image по умолчанию имя текущей директории
 * Переименовал --file в --dockerfile
 * Поддержка опций через конфиг файл
 * Возможность вызывать модуль

[Aleksey Ostapenko](http://staff/kbakba@yandex-team.ru) 2016-11-03 19:49:18

0.5
---

* [Kirill Sibirev](http://staff/sibirev@yandex-team.ru)

 * \-\-trace option for convert\_changelog\. FIx \#14

* [Dmitry Prokofyev](http://staff/dmirain@yandex-team.ru)

 * потерянный перенос строки
 * Небольшая защита от странных image\_url

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * Поддержал опции \-\-no\-commit, \-\-no\-push для команды release
 * Упразднил опцию \-\-application в пользу \-\-applications
 * Удалил Dockerfile и deps/
 * releasing version 0\.4

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2016-10-31 17:26:55

0.4
---

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * Команды release и stand
 * Команды build и push, подготовка к метакоманде release

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2016-10-28 16:41:39

0.3
---

* [Kirill Sibirev](http://staff/sibirev@yandex-team.ru)

 * http for staff\.yandex\-team\.ru
 * python\-click \+ дополнительные опции

* [Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru)

 * Remove dockerization
 * Исправил ошибку в alias
 * Команда deploy
 * Изменил имя пакета, добавил версию пакета
 * \-\-version option
 * gitchronicler==0\.3 \(escape markdown syntax, fixed bug with double quotes in commit message, explicit version\)
 * Добавил в README описание нюансов опции \-c
 * Мелкие праввки README
 * setup\.py
 * Поправил случай отказа от изменений в случае отсутствия changelog\.md в git
 * Переезд из registry\.ape\.yandex\.net в registry\.yandex\.net
 * Длинные и короткие версии опций
 * Поддержал опции \-remote и \-y
 * Первая версия
 * Initial commit

* [Azer Abdullaev](http://staff/buckstabu@yandex-team.ru)

 * Mount ssh agent socket into container
 * Changelog conversion option added

[Oleg Gulyaev](http://staff/o-gulyaev@yandex-team.ru) 2016-10-25 18:29:46

