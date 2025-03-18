// exhaustivestruct:ignore
package ignored

import (
	"exstr/inner"
)

type WholePackageIgnored struct {
	A int
	B float64
}

func thisMustStillBeChecked() inner.Inner {
	return inner.Inner{} // want "A, B are missing in `Inner` defined at inner/inner.go:3"
}
