Charts Notes
===

Добавление и удаление комментариев к [графикам](https://charts.yandex-team.ru)

В [`example`](example) есть пример работы. Эту цель можно запустить так: `Y_PYTHON_ENTRY_POINT=:repl example/example` и получить `IPython` с загруженным основным модулем под синонимом `notes`.

Чтобы начать работать, достаточно импортировать основной модуль:
```python
import library.python.charts_notes as notes
```

Термины:
* `feed` — канал. Например, для графика по ссылке https://charts.yandex-team.ru/preview/wizard/Users/voidex/offline_eta именем канала будет `Users/voidex/offline_eta`

Типы заметок:
* `notes.Line` — линия, привязана к дате
* `notes.Band` — область между датами
* `notes.Flag` — флаг, имеет высоту расположения
* `notes.Dot` — точка, привязана к линии (требуется `ID`)

Для аутентификации требуется OAuth-токен, который по умолчанию берётся из файла `~/.charts/token`. Также может быть передан явно в любую функцию.<br>
Заполучить токен можно [тут](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=09cea1cc285845b7b4dc3f409fcacad9)<br>
[Соответствующий раздел в документации](https://docs.charts.yandex-team.ru/api/auth)<br>

Создадим простой комментарий-линию:
```python
note_id = notes.create(
    feed='Users/voidex/offline_eta',
    date='2019-02-25',
    note=notes.Line(
        'lorem ipsum',
        color=notes.Colors.GREEN,
        dash_style=notes.DastStyle.DASH,
    ),
)
```

В качестве дат можно передавать как объекты типа `datetime.date`, так и строки, приемлемые для `dateutil.parse`.

Изменим заметку:
```python
note_id = notes.modify(
    note_id,
    note=notes.Line('lorem ipsum modified', color=notes.Color.RED),
)
```

И удалим:
```python
notes.delete(note_id)
```

Ссылки
====

https://docs.charts.yandex-team.ru/api/comments#redaktirovat-kommentarij
