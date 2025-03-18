package rule

import (
	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

type MessageErr struct {
	// Msg is text message describing this error.
	Msg string
	// Err is inner error returned from Unwrap.
	Err error
}

func (e *MessageErr) Error() string {
	return e.Msg
}

func (e *MessageErr) Unwrap() error {
	return e.Err
}

// Message rule wraps any rule error with custom message
func Message(msg string, rules ...Rule) Rule {
	return func(value *inspection.Inspected) error {
		for _, rule := range rules {
			if err := rule(value); err != nil {
				return &MessageErr{Msg: msg, Err: err}
			}
		}
		return nil
	}
}
