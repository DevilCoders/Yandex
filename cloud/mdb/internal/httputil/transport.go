package httputil

import (
	"bytes"
	"io"
	"io/ioutil"
	"net/http"
	"net/http/httputil"

	openapiclient "github.com/go-openapi/runtime/client"
	"github.com/opentracing-contrib/go-stdlib/nethttp"

	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/library/go/certifi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type loggingRoundTripper struct {
	l   log.Logger
	cfg LoggingConfig
	rt  http.RoundTripper
}

var _ http.RoundTripper = &loggingRoundTripper{}

type LoggingConfig struct {
	// LogRequestBody defines if request body should be logged.
	// ATTENTION: It logs EVERYTHING including authorization headers, etc. NOT FOR PRODUCTION USE.
	LogRequestBody bool `yaml:"log_request_body"`
	// LogResponseBody defines if response body should be logged
	// ATTENTION: It logs EVERYTHING including authorization headers, etc. NOT FOR PRODUCTION USE.
	LogResponseBody bool `yaml:"log_response_body"`
}

func DefaultLoggingConfig() LoggingConfig {
	return LoggingConfig{}
}

// LoggingRoundTripper wraps provided http.RoundTripper with a logging one
func LoggingRoundTripper(rt http.RoundTripper, cfg LoggingConfig, l log.Logger) http.RoundTripper {
	return &loggingRoundTripper{
		l:   l,
		cfg: cfg,
		rt:  rt,
	}
}

type TLSConfig struct {
	CAFile   string `yaml:"ca_file"`
	Insecure bool   `yaml:"insecure,omitempty"`
}

// DEPRECATEDNewTransport constructs http transport with customized TLS and logging
func DEPRECATEDNewTransport(tlscfg TLSConfig, logcfg LoggingConfig, l log.Logger) (http.RoundTripper, error) {
	var tlsOpts openapiclient.TLSClientOptions
	if tlscfg.CAFile == "" {
		var err error
		tlsOpts.LoadedCAPool, err = certifi.NewCertPoolSystem()
		if err != nil {
			return nil, xerrors.Errorf("failed to get system cert pool: %w", err)
		}
	} else {
		tlsOpts.CA = tlscfg.CAFile
	}
	rt, err := openapiclient.TLSTransport(
		tlsOpts,
	)
	if err != nil {
		return nil, xerrors.Errorf("failed to create HTTP transport: %w", err)
	}

	// Create transport with tracing and logging
	t := &nethttp.Transport{
		RoundTripper: LoggingRoundTripper(rt, logcfg, l),
	}
	return t, nil
}

type TransportConfig struct {
	TLS     TLSConfig     `json:"tls" yaml:"tls"`
	Retry   retry.Config  `json:"retry" yaml:"retry"`
	Logging LoggingConfig `json:"logging" yaml:"logging"`
}

func DefaultTransportConfig() TransportConfig {
	return TransportConfig{
		Retry: retry.Config{
			MaxRetries: 2,
		},
	}
}

// NewTransport constructs customized http transport
func NewTransport(cfg TransportConfig, l log.Logger) (http.RoundTripper, error) {
	var tlsOpts openapiclient.TLSClientOptions
	if cfg.TLS.CAFile == "" {
		var err error
		tlsOpts.LoadedCAPool, err = certifi.NewCertPoolSystem()
		if err != nil {
			return nil, xerrors.Errorf("failed to get system cert pool: %w", err)
		}
	} else {
		tlsOpts.CA = cfg.TLS.CAFile
	}
	tlsOpts.InsecureSkipVerify = cfg.TLS.Insecure
	rt, err := openapiclient.TLSTransport(
		tlsOpts,
	)
	if err != nil {
		return nil, xerrors.Errorf("failed to create HTTP transport: %w", err)
	}

	// Create transport with all the bells and whistles
	t := &nethttp.Transport{
		RoundTripper: LoggingRoundTripper(
			NewRetryRoundTripper(
				rt,
				retry.New(cfg.Retry),
				l,
			),
			cfg.Logging,
			l,
		),
	}
	return t, nil
}

// RoundTrip implements http.RoundTripper
// Does not error out when fails to log request or response
func (lrt *loggingRoundTripper) RoundTrip(r *http.Request) (*http.Response, error) {
	if lrt.cfg.LogRequestBody {
		dump, err := httputil.DumpRequestOut(r, true)
		if err != nil {
			ctxlog.Error(r.Context(), lrt.l, "failed to dump client request body", log.Error(err))
		} else {
			ctxlog.Debugf(r.Context(), lrt.l, "client request body:\n%s", dump)
		}
	}

	resp, err := lrt.rt.RoundTrip(r)
	if err != nil {
		return nil, err
	}

	if lrt.cfg.LogResponseBody {
		dump, err := httputil.DumpResponse(resp, true)
		if err != nil {
			ctxlog.Error(r.Context(), lrt.l, "failed to dump client response body", log.Error(err))
		} else {
			ctxlog.Debugf(r.Context(), lrt.l, "client response body:\n%s", dump)
		}
	}

	return resp, nil
}

type retryRoundTripper struct {
	l               log.Logger
	backoff         *retry.BackOff
	isRetryableReq  IsRetryableRequestFunc
	isRetryableResp IsRetryableResponseFunc
	rt              http.RoundTripper
}

type (
	IsRetryableRequestFunc  func(req *http.Request) bool
	IsRetryableResponseFunc func(resp *http.Response) bool
)

type RetryOption func(rrt *retryRoundTripper)

func WithIsRetryableRequestFunc(f IsRetryableRequestFunc) RetryOption {
	return func(rrt *retryRoundTripper) {
		rrt.isRetryableReq = f
	}
}

func WithIsRetryableResponseFunc(f IsRetryableResponseFunc) RetryOption {
	return func(rrt *retryRoundTripper) {
		rrt.isRetryableResp = f
	}
}

func DefaultIsRetryableRequest(req *http.Request) bool {
	return req.Method == http.MethodGet || req.Method == http.MethodHead
}

func DefaultIsRetryableResponse(resp *http.Response) bool {
	switch resp.StatusCode {
	case http.StatusInternalServerError, http.StatusBadGateway, http.StatusServiceUnavailable, http.StatusGatewayTimeout:
		return true
	}

	return false
}

func NewRetryRoundTripper(rt http.RoundTripper, backoff *retry.BackOff, l log.Logger, opts ...RetryOption) http.RoundTripper {
	rrt := &retryRoundTripper{rt: rt, backoff: backoff, l: l}

	for _, opt := range opts {
		opt(rrt)
	}

	// Set defaults if necessary
	if rrt.isRetryableReq == nil {
		rrt.isRetryableReq = DefaultIsRetryableRequest
	}
	if rrt.isRetryableResp == nil {
		rrt.isRetryableResp = DefaultIsRetryableResponse
	}

	return rrt
}

func (rrt *retryRoundTripper) RoundTrip(req *http.Request) (*http.Response, error) {
	// Check if request is retryable
	if !rrt.isRetryableReq(req) {
		return rrt.rt.RoundTrip(req)
	}

	// Copy body if needed
	var body *bytes.Reader
	if req.Body != nil {
		var buf bytes.Buffer
		if _, err := io.Copy(&buf, req.Body); err != nil {
			_ = req.Body.Close()
			return nil, err
		}
		_ = req.Body.Close()

		body = bytes.NewReader(buf.Bytes())
	}

	var resp *http.Response
	var respErr error
	// We ignore what retry function returns because we work with original error and response
	_ = rrt.backoff.RetryWithLog(
		req.Context(),
		func() error {
			// Reset body if any
			if body != nil {
				if _, serr := body.Seek(0, 0); serr != nil {
					return retry.Permanent(xerrors.New("permanent"))
				}
				req.Body = ioutil.NopCloser(body)
			}

			if resp != nil {
				_, _ = io.Copy(ioutil.Discard, resp.Body)
				_ = resp.Body.Close()
			}

			resp, respErr = rrt.rt.RoundTrip(req)
			// No error happened
			if respErr == nil {
				// We might want to retry this specific response
				if !rrt.isRetryableResp(resp) {
					// Happy path
					return nil
				}
			}

			return xerrors.New("temporary")
		},
		"HTTP roundtrip",
		rrt.l,
	)

	return resp, respErr
}

type HTTPConfig struct {
	TLS     TLSConfig     `yaml:"tls"`
	Logging LoggingConfig `yaml:"logging"`
}
