package requestid

import "github.com/gofrs/uuid"

func generateRequestID() string {
	return uuid.Must(uuid.NewV4()).String()
}

func parseValidUUID(s string) (string, error) {
	u, err := uuid.FromString(s)
	if err != nil {
		return "", err
	}

	return u.String(), err
}
