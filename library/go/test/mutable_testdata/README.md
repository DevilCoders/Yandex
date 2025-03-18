# mutable testdata

Бывают случаи, когда тесты библиотек в `vendor/` пытаются открывать на запись файлы из репозитория.
При запуске внутри CI такие тесты падают с ошибкой `Permission denied`.

Этот рецепт позволяет починить такие проблемы, без изменения в коде библиотеки.

Перед запуском теста, рецепт копирует указаные директории в рабочую директорию теста.

```
DATA(arcadia/vendor/github.com/foo/bar)

DEPENDS(library/go/test/mutable_testdata)

USE_RECIPE(library/go/test/mutable_testdata/mutable_testdata
   --testdata-dir vendor/github.com/foo/bar)
```
