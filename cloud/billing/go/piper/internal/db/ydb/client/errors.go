package ydbclient

import (
	"errors"
	"strings"

	"a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"
)

var (
	ErrConnection = errsentinel.New("connection")
	ErrPing       = errsentinel.New("database access")
)

type closeError struct {
	dbErr     error
	directErr error
}

func (c closeError) Error() string {
	bld := strings.Builder{}
	_, _ = bld.WriteString("close")
	if c.dbErr != nil {
		_, _ = bld.WriteString(" dbErr:'")
		_, _ = bld.WriteString(c.dbErr.Error())
		_, _ = bld.WriteString("'")
	}
	if c.directErr != nil {
		_, _ = bld.WriteString(" directPoolErr:'")
		_, _ = bld.WriteString(c.directErr.Error())
		_, _ = bld.WriteString("'")
	}
	return bld.String()
}

func (c closeError) Is(target error) bool {
	return (c.dbErr != nil && errors.Is(c.dbErr, target) ||
		c.directErr != nil && errors.Is(c.directErr, target))
}

func (c closeError) As(target interface{}) bool {
	return (c.dbErr != nil && errors.As(c.dbErr, target) ||
		c.directErr != nil && errors.As(c.directErr, target))
}
