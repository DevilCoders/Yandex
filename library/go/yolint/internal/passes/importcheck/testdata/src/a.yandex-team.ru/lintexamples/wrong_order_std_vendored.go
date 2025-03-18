package lintexamples // want `use yoimports to reformat file imports`

import ( // want `import groups must follow order: stdlib group then vendored then local group`
	_ "vendored.package/gofrs/uuid"

	_ "errors"
	_ "fmt"
	_ "log"
	_ "os"
)
