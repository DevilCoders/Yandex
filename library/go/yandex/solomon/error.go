package solomon

import (
	"errors"
	"fmt"
)

type APIError struct {
	Code    int    `json:"code"`
	Message string `json:"message"`
}

func (e *APIError) Error() string {
	return fmt.Sprintf("solomon: code=%d, message=%s", e.Code, e.Message)
}

var ErrNotFound = errors.New("not found")
