package httputil_test

import (
	"bytes"
	"net/http"
	"net/http/httptest"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestDefaultIsRetryableRequest(t *testing.T) {
	inputs := []struct {
		Method    string
		Retryable bool
	}{
		{},
		{Method: http.MethodGet, Retryable: true},
		{Method: http.MethodHead, Retryable: true},
		{Method: http.MethodPost},
		{Method: http.MethodPut},
		{Method: http.MethodPatch},
		{Method: http.MethodDelete},
		{Method: http.MethodConnect},
		{Method: http.MethodOptions},
		{Method: http.MethodTrace},
	}

	for _, input := range inputs {
		t.Run("Method "+input.Method, func(t *testing.T) {
			require.Equal(t, input.Retryable, httputil.DefaultIsRetryableRequest(&http.Request{Method: input.Method}))
		})
	}
}

func TestDefaultIsRetryableResponse(t *testing.T) {
	retryable := map[int]struct{}{
		http.StatusInternalServerError: {},
		http.StatusBadGateway:          {},
		http.StatusServiceUnavailable:  {},
		http.StatusGatewayTimeout:      {},
	}

	for status := range retryable {
		assert.Truef(t, httputil.DefaultIsRetryableResponse(&http.Response{StatusCode: status}), "status %d is expected to be retryable but it is not", status)
	}

	for status := 0; status < 1000; status++ {
		if _, ok := retryable[status]; ok {
			continue
		}

		assert.Falsef(t, httputil.DefaultIsRetryableResponse(&http.Response{StatusCode: status}), "status %d is expected to be non-retryable but it is", status)
	}
}

type Fixture struct {
	Client   *http.Client
	Server   *httptest.Server
	Attempts int
	Status   int
}

func newFixture(t *testing.T, status int, opts ...httputil.RetryOption) *Fixture {
	f := &Fixture{Status: status}

	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)

	// 2 retries (3 tries total in case of failures) and almost no wait at all
	b := retry.New(retry.Config{MaxRetries: 2, InitialInterval: time.Nanosecond, MaxInterval: time.Nanosecond})
	f.Client = &http.Client{Transport: httputil.NewRetryRoundTripper(http.DefaultTransport, b, l, opts...)}

	f.Server = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		f.Attempts++
		w.WriteHeader(status)
	}))

	t.Cleanup(func() {
		f.Server.Close()
	})

	return f
}

func (f *Fixture) Retried(t *testing.T, resp *http.Response, err error) {
	assert.NoError(t, err)
	assert.Equal(t, f.Status, resp.StatusCode)
	assert.Equal(t, 3, f.Attempts)
}

func (f *Fixture) NotRetried(t *testing.T, resp *http.Response, err error) {
	assert.NoError(t, err)
	assert.Equal(t, f.Status, resp.StatusCode)
	assert.Equal(t, 1, f.Attempts)
}

func TestLoggingRoundTripper_RoundTrip(t *testing.T) {
	t.Run("DefaultRetry", func(t *testing.T) {
		status := http.StatusBadGateway
		f := newFixture(t, status)
		resp, err := f.Client.Get(f.Server.URL)
		f.Retried(t, resp, err)
	})

	t.Run("DefaultNotRetryableRequest", func(t *testing.T) {
		status := http.StatusBadGateway
		f := newFixture(t, status)
		resp, err := f.Client.Post(f.Server.URL, "application/json", bytes.NewBuffer([]byte("{}")))
		f.NotRetried(t, resp, err)
	})

	t.Run("DefaultNotRetryableResponse", func(t *testing.T) {
		status := http.StatusForbidden
		f := newFixture(t, status)
		resp, err := f.Client.Get(f.Server.URL)
		f.NotRetried(t, resp, err)
	})

	t.Run("CustomRetryableResponse", func(t *testing.T) {
		status := http.StatusAlreadyReported
		f := newFixture(t, status, httputil.WithIsRetryableResponseFunc(func(resp *http.Response) bool {
			return resp.StatusCode == status
		}))
		resp, err := f.Client.Get(f.Server.URL)
		f.Retried(t, resp, err)
	})

	t.Run("CustomNotRetryableResponse", func(t *testing.T) {
		status := http.StatusAlreadyReported
		f := newFixture(t, status, httputil.WithIsRetryableResponseFunc(func(resp *http.Response) bool {
			return resp.StatusCode == http.StatusEarlyHints
		}))
		resp, err := f.Client.Get(f.Server.URL)
		f.NotRetried(t, resp, err)
	})
}
