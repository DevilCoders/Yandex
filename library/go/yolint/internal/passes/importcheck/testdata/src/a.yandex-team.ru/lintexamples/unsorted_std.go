package lintexamples // want `use yoimports to reformat file imports`

import (
	_ "log" // want `stdlib imports should be sorted alpabetically`
	_ "fmt"
	_ "errors"
	_ "os"
)
