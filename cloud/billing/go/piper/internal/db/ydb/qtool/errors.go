package qtool

import "fmt"

type ErrorWithQuery struct {
	query   string
	wrapped error
}

func WrapWithQuery(err error, query string) error {
	if err == nil {
		return err
	}
	return &ErrorWithQuery{
		query:   query,
		wrapped: err,
	}
}

func (err *ErrorWithQuery) Query() string {
	return err.query
}

func (err *ErrorWithQuery) Error() string {
	return fmt.Sprintf("%s\n%s", err.wrapped.Error(), err.query)
}

func (err *ErrorWithQuery) Unwrap() error {
	return err.wrapped
}
