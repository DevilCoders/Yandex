package dcs

import "fmt"

// NotFoundError is returned when specific path cannot be found in DCS.
type NotFoundError struct {
	path string
}

func NewNotFoundError(path string) NotFoundError {
	return NotFoundError{
		path: path,
	}
}

func (e NotFoundError) Error() string {
	return fmt.Sprintf("path %s cannot be found", e.path)
}
