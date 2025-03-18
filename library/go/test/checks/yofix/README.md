# Run yo fix

Этот рецепт запускает `ya tool yo fix` в режиме dry run и проверяет, что не было
внесено никаких изменений.

для использования ресепта создайте папку `yo_test` в том проекте, где нужны выполнять проверку
файлов `ya.make`.
в неё нужно добавить 2 файла:

в `ya.make` нужно:
- включить рецепт для запуска run yo fix
- указать в качестве `DATA` папку вашего проекта
- указать в качестве TEST_CWD папку вашего проекта
```yamake
GO_TEST()

OWNER(<OWNER>)

DATA(arcadia//<PROJECT_DIR>)

INCLUDE(${ARCADIA_ROOT}/library/go/test/checks/yofix/run.inc)

TEST_CWD(<PROJECT_DIR>)

GO_TEST_SRCS(yo_test.go)

END()
```

в `yo_test.go` нужно запустить yo fix и передать в параметрах владельца проекта.
```go
package yotest

import (
	yo_fix "a.yandex-team.ru/library/go/test/checks/yofix"
	"testing"
)

func TestRunYoFix(t *testing.T) {
	yo_fix.Run(t, "<OWNER>")
}
```
