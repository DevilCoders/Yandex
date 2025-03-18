# mkdocs_yandex

Плагин для [MkDocs](https://www.mkdocs.org/), который добавляет поддержку [YFM](https://wiki.yandex-team.ru/users/rudskoy/docs/arcadia-documenting/yfm/).
Обработка документа происходит в две стадии: прогон Jinja2 и рендеринг Markdown (подробности ниже).


## Установка

Внимание: нужен `setuptools` > 36.2.1

Устанавливаем из Аркадии:

    pip install svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/tools/mkdocs_builder/mkdocs_yandex#egg=mkdocs_yandex --user

Для локальной разработки:

    pip install -e .

## Подключение

Добавляем в `mkdocs.yml` такую строчку:

    plugins:
      - yandex

### Стадии обработки

#### Jinja2

В директории [ext/jinja2](./mkdocs_yandex/ext/jinja2) пишем [Jinja-расширения](http://jinja.pocoo.org/docs/2.10/extensions/#extension-api),
которые обрабатывают определенные тэги.
В расширении можно:

* Обработать тэг, заменив его на какой-либо MD/HTML.
  Во втором случае можно поставить у HTML-элемента атрибут `markdown="1"`, чтобы содержимое также было обработано на следующей стадии.
  [Пример: note](./mkdocs_yandex/ext/jinja2/note.py)
* Прокинуть тэг дальше без изменений, чтобы обработать его на следующей стадии. [Пример: inc](./mkdocs_yandex/ext/jinja2/inc.py)

#### Markdown

В директории [ext/markdown](./mkdocs_yandex/ext/markdown) пишем [Markdown-расширения](https://python-markdown.github.io/extensions/api/).
В расширении можно, например, создать `TreeProcessor`, который на вход принимает [ElementTree](http://effbot.org/zone/element-index.htm) и модифицирует его.
[Пример: list](./mkdocs_yandex/ext/markdown/list.py)
