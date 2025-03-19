package mon

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"io"
	"net/http"
	"sync"
	"time"
)

const (
	// DefaultTimeout is a default timeout for all checks if
	// it's not specified via WithTimeout
	DefaultTimeout = 5 * time.Second
	// OK is required to be returned by Check in case of success
	OK = "Ok"
)

var (
	// ErrExecutionTimeout returned as status if Check has not finished before deadline
	ErrExecutionTimeout = errors.New("execution check timeout")
	// ErrCheckPanicked returned when an excuted check ends up in panic
	ErrCheckPanicked = errors.New("check panicked")
)

const (
	// StatusOK meanse that MonRun check has finished w/o error
	StatusOK = "0"
	// StatusError is like StatusOK, but visa versa
	StatusError = "2"
)

type msg struct {
	Name  string
	Error error
}

// RepositoryOption configures underlinded repository
type RepositoryOption func(r *repository)

// WithTimeout sets a timeout for running checks.
// Checks are expected to be cancellable via Context
func WithTimeout(timeout time.Duration) RepositoryOption {
	return func(r *repository) {
		r.timeout = timeout
	}
}

// Check is a check registered in the repository to be run on request
type Check interface {
	Run(ctx context.Context) error
}

// CheckFunc converts plain functions to Check interface
type CheckFunc func(ctx context.Context) error

// Run implements Check
func (c CheckFunc) Run(ctx context.Context) error {
	return c(ctx)
}

// Repository is a place to register Checks
// Repository is responsible to run checks in parallel.
type Repository interface {
	// Add new Check. In case of duplicate name it panics
	// This method is thread-safe.
	Add(name string, check Check)
	// Remove a named check from Repository
	// Safe to remove a name that does not exist
	// thread-safe
	Remove(name string)
	// Run schedules checks and waits for results
	Run(ctx context.Context) CheckResult
	// Wraps Run into http.Handler
	http.Handler
}

// CheckResult ...
type CheckResult interface {
	IsOK() bool
	Description() string
}

type repository struct {
	sync.Mutex
	checks      map[string]Check
	timeout     time.Duration
	serviceName string
}

// NewRepository returns Repository
func NewRepository(name string, opts ...RepositoryOption) Repository {
	r := &repository{
		checks:      make(map[string]Check),
		serviceName: name,
		timeout:     DefaultTimeout,
	}
	for _, opt := range opts {
		opt(r)
	}

	return r
}

// Add implements Repository.Add
func (r *repository) Add(name string, c Check) {
	r.Lock()
	defer r.Unlock()
	if _, ok := r.checks[name]; ok {
		panic(fmt.Sprintf("duplicated check %s", name))
	}
	r.checks[name] = c
}

// Remove implements Repository.Remove
func (r *repository) Remove(name string) {
	r.Lock()
	defer r.Unlock()
	delete(r.checks, name)
}

//nolint:errcheck
func (r *repository) runChecks(ctx context.Context) <-chan msg {
	out := make(chan msg)
	var wg sync.WaitGroup
	r.Lock()
	defer r.Unlock()
	// Schedule all checks
	for k, v := range r.checks {
		wg.Add(1)
		go func(k string, f Check) {
			defer wg.Done()

			// NOTE: will be set after Run
			var err = ErrCheckPanicked
			func() {
				// NOTE: do not drop wrapper func, as recover()
				// must be evaluated after panic
				defer func() { recover() }()
				err = f.Run(ctx)
			}()

			result := msg{Name: k, Error: err}
			select {
			case out <- result:
				// pass
			case <-ctx.Done():
				// pass
			}
		}(k, v)
	}

	// Wait for the end
	go func() {
		wg.Wait()
		close(out)
	}()
	return out
}

func (r *repository) Run(ctx context.Context) CheckResult {
	r.Lock()
	strErrs := make(checkResult, len(r.checks))
	r.Unlock()

	// We don't want wait more then timeout or the client disconnects
	// NOTE: timeout is immutable
	ctx, cancel := context.WithTimeout(ctx, r.timeout)
	defer cancel()

	out := r.runChecks(ctx)

	for {
		select {
		case msg, ok := <-out:
			if !ok {
				return strErrs
			}
			strErrs[msg.Name] = msg.Error
		case <-ctx.Done():
			r.Lock()
			for k := range r.checks {
				if _, ok := strErrs[k]; !ok {
					strErrs[k] = ErrExecutionTimeout
				}
			}
			r.Unlock()
			return strErrs
		}
	}
}

// ServeHTTP implements http.Handler
func (r *repository) ServeHTTP(w http.ResponseWriter, req *http.Request) {
	w.Header().Set("Content-Type", "text/plain")

	cr := r.Run(req.Context())
	// NOTE: serviceName is immutable
	// MonRun format name;status;description
	buff := bytes.NewBufferString(r.serviceName)
	buff.WriteByte(';')
	if cr.IsOK() {
		buff.WriteString(StatusOK)
	} else {
		buff.WriteString(StatusError)
	}
	buff.WriteByte(';')
	buff.WriteString(cr.Description())

	//nolint:errcheck
	io.Copy(w, buff)
}

type checkResult map[string]error

func (c checkResult) IsOK() bool {
	for _, v := range c {
		if v != nil {
			return false
		}
	}
	return true
}

func (c checkResult) Description() string {
	var buff = new(bytes.Buffer)
	buff.WriteByte('{')
	var i = 0
	for name, err := range c {
		if i > 0 {
			buff.WriteByte(',')
		}
		i++
		buff.WriteByte('"')
		buff.WriteString(name)
		buff.WriteByte('"')
		buff.WriteByte(':')
		buff.WriteByte('"')
		if err == nil {
			buff.WriteString(OK)
		} else {
			buff.WriteString(err.Error())
		}
		buff.WriteByte('"')
	}
	buff.WriteByte('}')
	return buff.String()
}
