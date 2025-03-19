package api

import (
	"encoding/json"
	"errors"
)

type APIErr struct {
	StatusCode int
	Msg        string
	Err        error
}

func (e *APIErr) Error() string {
	var msg string
	if len(e.Msg) > 0 {
		msg = ": " + e.Msg
	}

	return e.Err.Error() + msg
}

func (e *APIErr) SetMsg(m string) (er *APIErr) {
	return &APIErr{
		e.StatusCode,
		m,
		e.Err,
	}
}

func (e *APIErr) Is(err error) bool {
	t, ok := err.(*APIErr)
	if !ok {
		return false
	}

	return (e.StatusCode == t.StatusCode) &&
		errors.Is(e.Err, t.Err)
}

func (e *APIErr) MarshalJSON() ([]byte, error) {
	return json.Marshal(e.Error())
}

// func (e *APIErr) Unwrap() error {
// 	return errors.New("s")
// }
