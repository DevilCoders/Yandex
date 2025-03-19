package tooling

import (
	"fmt"

	"github.com/gofrs/uuid"
)

func GenerateRequestID() string {
	return uuid.Must(uuid.NewV4()).String()
}

func PanicToError(err error, r interface{}) error {
	if r != nil {
		panicCall := getPanicCall()
		return fmt.Errorf("panic recovered: %v @ %s:%d(%s)",
			r, panicCall.File, panicCall.Line, panicCall.Function,
		)
	}
	return err
}
