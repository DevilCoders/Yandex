# yolint

## Как запустить yolint_next локально

Следующая версия линтера всегда доступна в аркадии для локального запуска. Включается флагом `-DGO_VET=yolint_next`.

```bash
ya make -r -DGO_VET=yolint_next -tt --test-type=govet
```

## Как запустить локальную сборку yolint.

```bash
go install a.yandex-team.ru/library/go/yolint/cmd/yolint
ya make -t -DGO_VET=local -DGO_VET_TOOL="$(which yolint)" -DGO_FAKEID="$(sha1sum $(which yolint))" path/to/my/project
```

_Если вы тестируете собственный линтер, то это нужно делать через [analysistest](https://godoc.org/golang.org/x/tools/go/analysis/analysistest), а не через ручной запуск._

## Добавление нового стороннего линтера

1. Добавить `mylinter.Analysis` в [`main.go`](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yolint/cmd/yolint/main.go).
2. Если в линтере ожидается много проблем, то стоит обернуть его в `middlewares.Migration`.
3. Если линтер должен поддерживать nolint, то обернуть его в `middlewares.NoLint`.
4. Если линтер должен быть отключен в генерённом коде, то обернуть его в `middlewares.NoGen`
5. Послать код на ревью.

## Обновление yolint

1. Склонировать и запустить задачу https://sandbox.yandex-team.ru/task/1250390812/view (не забыть удалить указание конкретной ревизии из поля `Svn url for arcadia`, иначе соберётся старая ревизия).
2. Скопировать id ресурсов `ARCADIA_PROJECT_TGZ` из подзадач и вставить в [ya.make](https://a.yandex-team.ru/arc/trunk/arcadia/build/external_resources/yolint/ya.make).
3. _Опционально._ Отредактировать флаги запуска линтера в [ymake.core.conf](https://a.yandex-team.ru/arc/trunk/arcadia/build/ymake.core.conf?rev=5821724#L5663)

_Обновление yolint нужно делать через PR. Нельзя вливать yolint, который создаёт новые style ошибки. Ошибки новых линтеров всегда нужно чинить в транке перед включением._

## scopelint

Существет возможность включения дополнительных линтеров для отдельных директорий в Аркадии. Для этого необходимо прописать в [специальный конфиг](https://a.yandex-team.ru/arc_vcs/build/rules/go/extended_lint.yaml) название вашего пакета в Аркадии и список требуемых линтеров:

```yaml
---
scopes:
  a.yandex-team.ru/<myproject>/<mypackage>:
    - importcheck
    - SA1019
```

Список доступных линтеров не включенных по-умолчанию для всей Аркадии:

- `importcheck` - проверяет правильность форматирования блока импортов в файле
- `copyproto` - запрещает [прямое копирование](https://clubs.at.yandex-team.ru/arcadia/22420) proto сообщений
- `bodyclose` - проверяет, что тело HTTP запроса было закрыто
- `rowserrcheck` - проверяет, что был вызван метод `(database/sql.Rows).Err`
- `sqlclosecheck` - проверяет, что был вызван метод `Close` для `sql.Rows` и `sql.Stmt`
- `ytcheck` - проверяет, что был вызван метод `(a.yandex-team.ru/yt/go/yt.TableReader).Err`
- `SA1019` - запрещает использование `deprecated` методов и объектов
- `ST1000` - проверяет наличие комментария на уровне пакета
- `ST1020` - проверяет наличие комментария у экспортированной функции
- `ST1021` - проверяет наличие комментария у экспортированного типа
- `ST1022` - проверяет наличие комментария у экспортированной переменной
- `SA6002` - проверяет, что объекты передаются в `sync.Pool` по указателю
- `SA4005` - проверяет случай, в котором изменения вносятся в поля структуры, переданной по значению
