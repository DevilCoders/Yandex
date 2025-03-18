// want package:"exhaustivestruct-to-be-checked"
// exhaustivestruct:skip
package skipped

import "exstr/inner"

type StillToBeChecked struct {
	A int
	B int
}

func checkIsSkipped() inner.Inner {
	return inner.Inner{}
}
