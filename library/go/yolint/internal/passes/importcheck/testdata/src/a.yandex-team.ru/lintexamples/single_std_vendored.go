package lintexamples // want `use yoimports to reformat file imports`

import (
	_ "errors"
	_ "fmt"
	_ "log"
	_ "os"
	_ "vendored.package/gofrs/uuid" // want `import groups should be divided by blank line`
)
