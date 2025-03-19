# WF Template

Шаблонизатор витрин. Выполняет следующие действия:

- Создание витрины. Создает по шаблону в модуль с витринами, обновляет `ya.make` файлы
по пути к модулю и в `vh/ya.make`.
- Удаление витрины. Удаляет папку модуля, убирает из `ya.make` файлов удаляемый модуль.

## Как запускать

Скрипт работает **исключительно** при запуске из папки в репозитории, поэтому перейдите в неё предварительно:

```
/root/cloud/dwh/nirvana
```

Создать витрину `new_module` с тегом `tag` в папке `/root/cloud/dwh/nirvana/vh/workflows/cdm/yt`:

```commandline
python3 ./wf-template/wf-template.py --module cdm.yt.new_module create --tags tag
```

Удалить витрину `new_module` из папки `/root/cloud/dwh/nirvana/vh/workflows/cdm/yt`:

```commandline
python3 ./wf-template/wf-template.py --module cdm.yt.new_module delete
```

## Описание параметров CLI

### Общие параметры

`--module` - путь из папки `/root/cloud/dwh/nirvana/vh/workflows` до модуля.

Пример: `--module cdm.yt.support.dm_yc_support_tag_issues`

Последним разделом является модуль, над которым будет производиться операция создания или удаления.

### Создание

`--tags` (опционально) - теги для витрины, указанные через пробел. Дубликаты тегов будут исключены.

Пример: `--tags tag1 tag2 tag3`

### Удаление

Нет уникальных параметров.
