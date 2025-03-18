package lintexamples // want `use yoimports to reformat file imports`

import ( // want `import groups must follow order: stdlib group then vendored then local group`
	_ "a.yandex-team.ru/lib"
	_ "errors"
	_ "vendored.package/gofrs/uuid"
	_ "fmt"

	_ "log"

	_ "os"
)
