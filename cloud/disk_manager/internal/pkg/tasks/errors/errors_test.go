package errors

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"
)

////////////////////////////////////////////////////////////////////////////////

func TestRetriableAndNonRetriableErrorsShouldUnwrapCorrectly(t *testing.T) {
	err := assert.AnError

	assert.True(
		t,
		Is(
			&NonRetriableError{Err: &RetriableError{Err: err}},
			&NonRetriableError{},
		),
	)
	assert.True(
		t,
		Is(
			&NonRetriableError{Err: &RetriableError{Err: err}},
			err,
		),
	)
	assert.True(
		t,
		Is(
			&NonRetriableError{Err: &RetriableError{Err: err}},
			&RetriableError{},
		),
	)
	assert.True(
		t,
		Is(
			&RetriableError{Err: &NonRetriableError{Err: err}},
			&NonRetriableError{},
		),
	)
	assert.True(
		t,
		Is(
			&RetriableError{Err: &NonRetriableError{Err: err}},
			&NonRetriableError{},
		),
	)
	assert.True(
		t,
		Is(
			&RetriableError{Err: &NonRetriableError{Err: err}},
			err,
		),
	)
	assert.True(
		t,
		Is(
			&NonRetriableError{Err: &NonCancellableError{Err: err}},
			&NonCancellableError{},
		),
	)

	assert.False(
		t,
		Is(
			&NonRetriableError{Err: &NonRetriableError{Err: err}},
			&RetriableError{},
		),
	)
	assert.False(
		t,
		Is(
			&RetriableError{Err: &RetriableError{Err: err}},
			&NonRetriableError{},
		),
	)
	assert.False(
		t,
		Is(
			&NonRetriableError{Err: &RetriableError{Err: fmt.Errorf("other error")}},
			err,
		),
	)
	assert.False(
		t,
		Is(
			&NonRetriableError{Err: &RetriableError{Err: err}},
			&NonCancellableError{},
		),
	)
}

func TestFoundErrorShouldUnwrapCorrectly(t *testing.T) {
	assert.True(
		t,
		Is(
			&NonRetriableError{Err: &NotFoundError{}},
			&NotFoundError{},
		),
	)
	assert.True(
		t,
		Is(
			&NonRetriableError{Err: &NotFoundError{TaskID: "id"}},
			&NotFoundError{},
		),
	)
}

func TestObtainDetailsFromDetailedError(t *testing.T) {
	details := &ErrorDetails{
		Code:     1,
		Message:  "message",
		Internal: true,
	}

	e := &DetailedError{}

	// Details should be obtained correctly even if DetailedError is wrapped
	// into some other error.
	assert.True(
		t,
		As(
			&NonRetriableError{
				Err: &DetailedError{
					Err:     assert.AnError,
					Details: details,
				},
			},
			&e,
		),
	)

	assert.Equal(t, assert.AnError, e.Err)
	assert.Equal(t, details, e.Details)
}

func TestCanRetry(t *testing.T) {
	err := assert.AnError

	assert.True(t, CanRetry(&RetriableError{Err: err}))

	assert.False(t, CanRetry(err))
	assert.False(t, CanRetry(&RetriableError{Err: &NonCancellableError{Err: err}}))
	assert.False(t, CanRetry(&RetriableError{Err: &NonRetriableError{Err: err}}))
}

func TestNonRetriableErrorSilent(t *testing.T) {
	err := &NonRetriableError{
		Err:    assert.AnError,
		Silent: true,
	}

	e := &NonRetriableError{}
	assert.True(t, As(err, &e))
	assert.True(t, e.Silent)
}

func TestNonRetriableErrorSilentUnwrapsCorrectly(t *testing.T) {
	err := &RetriableError{
		Err: &NonRetriableError{
			Err:    assert.AnError,
			Silent: true,
		},
	}

	e := &NonRetriableError{}
	assert.True(t, As(err, &e))
	assert.True(t, e.Silent)
}

func TestRetriableErrorIgnoreRetryLimitUnwrapsCorrectly(t *testing.T) {
	err := &RetriableError{
		Err: &NonRetriableError{
			Err: assert.AnError,
		},
		IgnoreRetryLimit: true,
	}

	e := &RetriableError{}
	assert.True(t, As(err, &e))
	assert.True(t, e.IgnoreRetryLimit)
}

////////////////////////////////////////////////////////////////////////////////

type simpleError struct{}

func (simpleError) Error() string {
	return "Simple error"
}

func TestSimpleErrorUnwrapsCorrectly(t *testing.T) {
	assert.True(t, Is(&NonRetriableError{Err: &simpleError{}}, &simpleError{}))
}
