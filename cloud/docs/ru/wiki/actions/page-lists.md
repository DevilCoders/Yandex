# Списки страниц

Вы можете вставить на [вики-страницу](../pages-types.md#page) списки:

* [{#T}](#tree)
* [{#T}](#backlinks)
* [{#T}](#mypages)

## Страницы раздела {#tree}

Блок содержит список всех страниц заданного раздела.

### Вызов блока {#tree-call}

```
{{tree for="адресСтраницы" nomark="0" depth="3" show_redirects="False" show_grids="True" show_files="False" show_owners="False" show_titles="True" show_created_at="False" show_modified_at="False" sort_by="title" sort="asc"}}
```

Все параметры блока `not_var{{tree}}` необязательные, в примере приведены значения параметров по умолчанию.

### Параметры блока (необязательные) {#tree-params}

Параметр | Описание
--- | ---
`for` | Адрес страницы, для которой строится дерево. 
`page` | Альтернативное имя (алиас) параметра `for`.
`depth` | Глубина дерева. Если равна `0`, то глубина не ограничена.
{% if audience == "draft" %}`show_redirects` | Отображение редиректов в дереве:<br>`true` — редиректы включены в дерево;<br>`false` — редиректы не отображаются в дереве.{% endif %}
`show_grids` | Отображение динамических таблиц в дереве:<br>`true` — динамические таблицы включены в дерево;<br>`false` — динамические таблицы не отображаются в дереве.
`show_files` | Отображение прикрепленных файлов в дереве:<br>`true` — файлы включены в дерево;<br>`false` — файлы не отображаются в дереве.
`show_owners` | Отображение авторов страниц:<br>`true` — авторы отображаются;<br>`false` — авторы скрыты.
`show_titles` | Отображение заголовков страниц:<br>`true` — заголовки отображаются;<br>`false` — заголовки скрыты. 
`show_created_at` | Отображение дат создания страниц:<br>`true` — даты отображаются;<br>`false` — даты скрыты.
`show_modified_at` | Отображение дат изменения страниц:<br>`true` — даты отображаются;<br>`false` — даты скрыты.
`sort_by` | Сортировать страницы:<br>`title` — по заголовку;<br>`cluster` — по разделу;<br>`created_at` — по дате создания;<br>`modified_at` — по дате изменения.
`sort` | Порядок сортировки:<br>`asc` — по возрастанию;<br>`desc` — по убыванию.

## Страницы, ссылающиеся на заданную {#backlinks}

Блок содержит список всех страниц, на которых присутствуют ссылки на заданную страницу.

### Вызов блока {#backlink-call}

```
{{backLinks for="адресСтраницы"}}
```

### Параметры блока (необязательные) {#baclink-params}

Параметр | Описание
--- | ---
`for` | Содержит адрес страницы, для которой строится список. Если параметр не указан, список строится для текущей страницы.

## Страницы, для которых я автор {#mypages}

Блок содержит список страниц, автором которых является пользователь.

### Вызов блока {#mypages-call}

```
not_var{{mypages}}
```

Все параметры блока `not_var{{mypages}}` необязательные. По умолчанию в блоке выводится список страниц того пользователя, который его просматривает. Страницы выводятся в алфавитном порядке.

### Параметры блока (необязательные) {#mypages-params}

Параметр | Описание
--- | ---
`user` | Вы можете вывести список страниц любого пользователя, указав в параметре `user` его логин.
`bydate` | Сортировка по дате создания:<br>`1` — включить;<br>`0` — отключить.
`bychange` | Сортировка по дате изменения:<br>`1` — включить;<br>`0` — отключить.

Например, чтобы вывести список страниц пользователя `username` и отсортировать их по дате изменения, задайте блок:

```
{{mypages bychange=1 user="username"}}
```