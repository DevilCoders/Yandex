package logbroker

import (
	"context"
	"errors"

	"a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
)

var (
	ErrAlreadyRunning = errors.New("service already running")
	ErrStillRunning   = errors.New("service still running")

	ErrConsumerFailed = errsentinel.New("logbroker consumer fatal error")

	ErrMissconfigured = errsentinel.New("configuration error")
	ErrWriterInit     = errsentinel.New("writer init error")
	ErrWrite          = errsentinel.New("write error")
)

func GetWriteIssues(err error) []*persqueue.Issue {
	var ie issuesError
	errors.As(err, &ie)
	return ie.Issues
}

type issuesError struct {
	error

	Issues []*persqueue.Issue
}

func withIssues(err error, issues []*persqueue.Issue) error {
	if err == nil || errors.Is(err, context.Canceled) || len(issues) == 0 {
		return err
	}
	return &issuesError{
		error:  err,
		Issues: issues,
	}
}

func (ie *issuesError) As(tgt interface{}) bool {
	if target, ok := tgt.(*issuesError); ok {
		target.error = ie.error
		target.Issues = append(target.Issues, ie.Issues...)
		return true
	}
	return false
}

func (ie *issuesError) Unwrap() error {
	return ie.error
}
