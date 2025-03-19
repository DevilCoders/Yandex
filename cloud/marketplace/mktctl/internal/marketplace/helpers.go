package marketplace

import "github.com/gofrs/uuid"

func newRequestID() (string, error) {
	id, err := uuid.NewV4()

	return id.String(), err
}
