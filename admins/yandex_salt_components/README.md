# ✨Аркадизированный✨ Upstream Salt 🔮

## Терминология
Чтобы мы понимали друг друга, нужно объяснить несколько терминов
- `модуль python` - это стандартный питон модуль, код который `python` импортирует через `import`
- `модуль salt-а` - это тоже стандартный python модуль, но salt грузит его хитрой магией `importlib`
- `герметичный` - не зависящий от внешнего окружения (честно! совсем не зависящий)
- `вкомпилен` - упакован в бинарник в качестве данных или кода
- `кастомный модуль` - это модуль расширения salt-а ([module][saltmods], [grains][saltgrains], [state][saltstate] ... [любой из][module_types]) добавляющий к нему новую функциональность
- `upstream salt` - код ghtub.com/saltstack/salt взятый из официальных релизов
- `patchset` - каталог с патчами к релизному коду upstream salt

Если ещё что-то не понятно, пожалуйста обратитесь к [документации salt-а][saltdoc]

## Что это и зачем

Часто при наливке возникает проблема с деплоем пакетов salt-а, казалось бы простая задача
- добавить нужные репозитории
- поставить пакет, который притянет все нужные зависимости

Превращается в кучу проблем:
- доставка реп под конкретные дистрибутивы
- резолвинг отсутствия и конфликтов зависимостей
- неподдерживаемая версия python на старых дистрибутивах
- невозможность легко обновиться на upstream где пофиксили кучу багов

И боль от них усиливается с каждым новым upstream релизом и с каждым новым задеплоенным
кластером.

Многим известна попытка форкнуть upstream salt и собирать пакет по мере обновления версий.<br/>
Репозитории тут [github.yandex-team.ru/Salt/salt][Salt/salt].<br/>
Проблемы с ней были в том, что это весь код salt-а, его много и накладывать свои патчи прямо на него трудно.<br/>
Вот несколько проблем, которые возникали с этим форком:
- после обновления на очередной релиз приходится фиксить конфликты от своих патчей
- приходится резолвить зависимости для поддержки кучи разных дистрибутивов
- кастомные модули нужно писать по "правилам" salt-а

Всё это сподвигло нас сделать подобие аркадизации для upstream salt-а.

## Что умеет этот пакет

В этом каталоге лежит результат усилий по аркадизации upstream салта.<br/>
Из самых заметных фич:
- установка одним пакетом `yandex-salt-components`
- герметичность, все импорты только из бинарника, нет хождения по файловой системе
- самый свежий питон, тот что в аркадии (на момент написания ридми 3.9.9)
- yandex-components так же вкомпилены в бинарь и могу использовать всю аркадию
- кастомные модули можно писать не оглядываясь на salt (не используя всякие `six` и `salt.utils.*`)
- использование самой современной крипты (из вариантов предоставляемых салтом) `M2Crypto`

В итоге получаем лёгкую установку, везде одинаковое поведение и самые свежие фичи (и баги конечно 😂)

Вместе с миграцией всех кастомных модулей salt из [нашего форка][Salt/salt] мы добавили модули для [dynamic-roots][dynamic_roots] ([наша дока про них][dynrootsdoc])

Текущий список кастомных модулей такой
```
fileserver/dynamic_roots.py
pillar/dynamic_roots.py

grains/cauth.py
grains/conductor.py
grains/yandex_common.py

modules/conductor.py
modules/ldmerge.py
modules/yav.py

states/monrun.py
states/yafile.py
states/yanetconfig.py
```

Стейт `states/yanetconfig.py` пока что не рабочий до миграции python-netconfig в аркадию ARCADIA-2547

## Как начать использовать

- Достаточно поставить одним пакетом `yandex-salt-components` мастеров, и настроить их.
- Поставить везде этим же пакетом миньонов и настроить.
- 🪄 ✨**Всё!**✨
- Можно пользоваться!

Всё остальное работает как и ожидается в стандартной инсталляции салта, но с маленькими особенностями, с которыми вы скорее всего не столкнётесь.

## Особенности
Приведу их списком

* Модули вкомпиленные в бинарь (наши кастомные) не могут быть переопределены через `_modules, _states, _grains` в `file_roots`,
  мы считаем это преимуществом!
* Так как нет походов в Фс на этапе питонячих `import`-ов, салт может использовать только те питон модули, которые вкомпилены в бинарник.<br/>
  Это значит, что нельзя поставить `pip`-ом на систему какой-то пакет и потом использовать его в кастомном модуле или в самом `salt`<br/>
  **Правильно добавить всё нужное в сборку в этом пакете.**

## Как это работает
Идея очень простая!

- делаем `PY_PROGRAM` (бинарник), который содержит в себе все необходимые питон модули для работы upstream salt-а, вот [ya.make][ya_make].
- добавляем кастомных модулей прямо в сборку бинарника.
- притаскиваем код upstream salt-а из релиза и пакуем в пакет с бинарником, вот [конфигурация для ya packages][ya_packages_conf]
- в пакете `python модуль salt` лежит в `/opt/yandex_salt_components/salt/`
- интерпритатор питона едет с нашим бинарником! (это и есть наш бинарник)
- разрешаем бинарнику грузить всего один python модуль с файловой системы - это модуль `salt` и он находится в сторонке от всего остального питона.
- делаем symlink-и `salt-master,salt-minion,salt-ssh,salt-key` на бинарник из этого пакета,
  так он будет понимать, какую часть мы хотим запустить (почти так же делают оригинальные пакеты).
- **пользуемся**!!!

## Добавление кастомного модуля в сборку

Если хочется расширить функционал salt-а каким-то своим модулем [например grain-ом][saltgrains],
или любым [из списка][module_types], то...
### Пишем код
Нужно расположить его правильно в файловой системе этого пакета.
Дальше написать модуль по правилам (например для [state][saltstate]),
но тут можно избегать использования `salt.utils` и `salt.six`, всего того,
что нужно для поддержки множества версий питона и не герметичного окружения.

Смотрите на примеры из этого пакета (текущий список доступных модулей см. выше)

### Помним про импорты `python` модуля `salt`-а
Второй важный пункт, импорты из `python` пакета `salt` не должны быть на верхнем уровне,
вы поймёте почему, запустим `ya make -t`.<br/>
Потому что в аркадии нет `salt-а` и связывание (импорт) будет происходить уже динамически с файловой системы,
где будет запускаться бинарник, но это всё ещё герметично по отношению к окружению.

### Выпрямляем `import`-ы
Третий шаг сложный, нужно переизобрести `TOP_LEVEL` из аркадийной сборки питона, так как `TOP_LEVEL` запретили,
делаем по аналогии что-то подобное, пример для [pillar-ов][reimport_pillar] (смотреть в [`monkey_patching.py`][monkey_patching])

```(python)
def reimport_pillar():
    from admins.yandex_salt_components import pillar
    from admins.yandex_salt_components.pillar import dynamic_roots

    sys.modules["pillar"] = pillar
    sys.modules["pillar.dynamic_roots"] = dynamic_roots


```

### Добавляем в список reimport-ов
И если мы создавали какой-то новый тип модулей, добавляем его в список
```(python)
def patch_upsteam_salt():
    # Adding the trace method to the logger as a side effect
    # Therefore, the logger must be imported strictly after this call
    assert "logging" not in sys.modules
    monkeypath_salt_lazy_loader()

    reimport_fileserver()
    reimport_pillar()   # <<<- сюда например
    reimport_grains()
    reimport_modules()
    reimport_states()
```

### Рассказываем salt's LazyLoader-у как импортнуть наш модуль
Последний шаг, тоже не простой и он связан с monkey патчингом `LazyLoader`-а.<br/>
Нужно подсказать LazyLoader-у о том, что наш модуль вкомпилен в бинарь.<br/>
Делаем по аналогии в функции [`monkeypath_salt_lazy_loader`][monkeypath_salt_lazy_loader]
```(python)
def monkeypath_salt_lazy_loader():
    # allow to load external salt modules
    sys.path.append(get_salt_module_path())  # <<< NOTE: тут вся магия по загрузке кода из релиза upstream salt-а
    from salt.loader import lazy

    old_refresh = lazy.LazyLoader._refresh_file_mapping

    def _refresh_file_mapping(self):
        old_refresh(self)
        ...
        elif self.tag == "pillar":
            self.file_mapping["dynamic_roots"] = ("pillar.dynamic_roots", ".o", 0)  # <<< Вот так
        ...

    lazy.LazyLoader._refresh_file_mapping = _refresh_file_mapping
```

### Релизим
Конечно же добавляем тесты на наш новый модуль!<br/>
Запускаем `ya make -t` если всё ок, то бампаем версию в [pkg.json][ya_packages_conf]<br/>
Коммитим и идём в [CI][ci_yandex_salt_components]<br/>
Делаем там новый релиз, тестируем его где-нибудь в тестинге!<br/>
Если всё ок, `dmove`-аем его на `dist`-e в `stable` ветку<br/>

## Почему это работает

✨Без магии✨ тут не обошлось, но реальных магических фраз,
с помощью которых этого удалось добиться я не расскажу.

Salt не грузит все модули сразу при запуске `salt-call` или `salt-minion`,
у него есть `LazyLoader` который ходит в файловую систему внутри
`python` пакета `salt`, только тогда, когда код обратился к какому-то salt модулю.

Если salt модуль найден LazyLoader-ом, он отдаётся пользователю, если нет, то выкидывается ошибка.<br/>
В коде upstream salt-а есть зачатки для использования вкомпиленных модулей,
но они находятся в каком-то странном не юзабельном состоянии и поэтому при
аркадизации пришлось немного попатчить этот самый `LazyLoader`.

В будущем можно попробовать пропихнуть в upstream такой патч, который
позволит избавиться от [`monkey_patching.py`][monkey_patching].
Но это кажется не сильно важным предприятием, куда важнее фиксить баги,
которые нашлись в upstream salt, об этом я расскажу в следующей секции.

## Как фиксить баги в upstream salt

Конечно нужно делать Pull Request в upstream salt!<br/>
Но ждать когда этот PR примут и ничего не делать у себя было бы странным,
поэтому было решено добавить `patchset`.<br/>

### Пишем патч
[patchset][patchset] - это каталог в котором лежит набор патчей сделанных `git diff --no-prefix` в каталоге с `python` модулем `salt`-а<br/>
Патч делается строго на код релиза, если патч перестаёт накладываться на следующий релиз, нужно его либо удалять либо фиксить!<br/>
Все патчи подкрепляются PR-ками в upstream salt!<br/>
Вот примерный workflow создания патча
```(shell)
TEMPDIR="$(mktemp -d)"
cd $TEMPDIR

RELEASE="3004"
FILE="salt-${RELEASE}.tar.gz"
URL="https://github.com/saltstack/salt/releases/download/v$RELEASE/$FILE"
curl -LOL "$URL"
tar xzf $FILE
pushd "${FILE%%.tar.gz}"
git init
git add .
git commit -m "init"
#
# Делаем фиксы в код
# Делаем
# Делаем
# Делаем
#
git diff --no-prefix > /path/to/the/arcadia/admins/yandex_salt_components/patchset/000N-file_name.py.patch
cd /path/to/the/arcadia/admins/yandex_salt_components
arc add patchset
arc commit ....
...
```

### Применяем патч, делаем sandbox ресурс для ya package
Да, это ещё не всё!

NOTE: в системе должна присутствовать утилита `patch`.

Теперь нам нужно запустить [./make-sandbox-resource.sh 'SALT RELEASE NUMBER'][resource_maker]
(да да, это планируеся перенести в New CI / Sandbox)<br/>
Запуск `./make-sandbox-resource.sh` сделает почти то же самое что мы сделали выше, но с небольшими дополнениями:
- скачает релиз 
- распакует его
- наложит на него патчи из [patchset][patchset]
- запакует всё обратно
- зальёт в sandbox

Не забудьте поставить `ttl: inf` у полученного ресурса,
не делаем это автоматически, потому что не всегда это нужно.<br/>
После переноса этой рутины в CI будем делать всё автоматом.

### Обновляем `pkg.json`

Теперь нам нужно взять `id` получившегося ресурса в `sandbox` и обновить [pkg.json][ya_packages_conf], так же бампаем версию пакета.

Дальше релизим, всё как при добавлении кастомного модуля

Спасибо, что дочитали!


  [Salt/salt]: <https://github.yandex-team.ru/Salt/salt/>
  [dynamic_roots]: <https://a.yandex-team.ru/arc_vcs/admins/yandex_salt_components/fileserver/dynamic_roots.py>
  [dynrootsdoc]: <https://wiki.yandex-team.ru/noc/nocdev/ops/salt/>
  [ya_make]: <https://a.yandex-team.ru/arc_vcs/admins/yandex_salt_components/ya.make>
  [saltdoc]: <https://docs.saltproject.io/en/getstarted/>
  [saltmods]: <https://docs.saltproject.io/en/latest/ref/modules/index.html>
  [saltgrains]: <https://docs.saltproject.io/en/latest/topics/grains/index.html#writing-grains>
  [saltstate]: <https://docs.saltproject.io/en/latest/ref/states/writing.html#using-custom-state-modules>
  [module_types]: <https://docs.saltproject.io/en/latest/topics/development/modules/index.html#module-types>
  [ya_packages_conf]: <https://a.yandex-team.ru/arc_vcs/admins/yandex_salt_components/pkg.json>
  [monkey_patching]: <https://a.yandex-team.ru/arc_vcs/admins/yandex_salt_components/monkey_patching.py?rev=adcd3140b581b2ac99839016a4553ecf59f82974#L67>
  [patchset]: <https://a.yandex-team.ru/arc_vcs/admins/yandex_salt_components/patchset/>
  [resource_maker]: <https://a.yandex-team.ru/arc_vcs/admins/yandex_salt_components/make-sandbox-resource.sh>
  [reimport_pillar]: <https://a.yandex-team.ru/arc_vcs/admins/yandex_salt_components/monkey_patching.py?rev=adcd3140b581b2ac99839016a4553ecf59f82974#L58>
  [monkeypath_salt_lazy_loader]: <https://a.yandex-team.ru/arc_vcs/admins/yandex_salt_components/monkey_patching.py?rev=adcd3140b581b2ac99839016a4553ecf59f82974#L84>
  [ci_yandex_salt_components]: <https://a.yandex-team.ru/projects/nocdev/ci/releases/timeline?dir=admins%2Fyandex_salt_components&id=release>
