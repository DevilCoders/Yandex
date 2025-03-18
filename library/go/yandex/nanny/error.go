package nanny

import (
	"errors"
	"fmt"
	"net/http"
	"strings"
)

type MethodError struct {
	Method string
	Path   string
	Inner  error
}

func (e *MethodError) Error() string {
	return fmt.Sprintf("nanny: method=%s, path=%s: %v", e.Method, e.Path, e.Inner)
}

func (e *MethodError) Unwrap() error {
	return e.Inner
}

type APIError struct {
	StatusCode int `json:"-"`

	ErrorString string `json:"error"`
	Message     string `json:"msg"`
}

func (e *APIError) Error() string {
	return fmt.Sprintf("nanny: code=%d, error=%q, msg=%q", e.StatusCode, e.ErrorString, e.Message)
}

func IsNotFound(err error) bool {
	var apiErr *APIError
	return errors.As(err, &apiErr) && apiErr.StatusCode == http.StatusNotFound
}

func IsConflict(err error) bool {
	var apiErr *APIError
	return errors.As(err, &apiErr) && apiErr.StatusCode == http.StatusConflict
}

type ValidationError struct {
	Path    []string `json:"path"`
	Message string   `json:"message"`
}

func (e *ValidationError) Error() string {
	return fmt.Sprintf("validation error: path=%s, message=%q", strings.Join(e.Path, "."), e.Message)
}
