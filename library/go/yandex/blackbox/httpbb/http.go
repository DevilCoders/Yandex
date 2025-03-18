package httpbb

import (
	"context"
	"crypto/tls"
	"encoding/json"
	"fmt"
	"net/http"
	"time"

	"github.com/cenkalti/backoff/v4"
	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/library/go/certifi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb/bbtypes"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type HTTPClient struct {
	httpc   *resty.Client
	tvmc    tvm.Client
	log     log.Structured
	retries uint64
	env     Environment
}

func newHTTPClient(env Environment, r *resty.Client) *HTTPClient {
	return &HTTPClient{
		httpc: r.
			SetBaseURL(env.BlackboxHost).
			SetTimeout(defaultTimeout).
			SetRedirectPolicy(resty.NoRedirectPolicy()).
			SetHeader("User-Agent", "BlackBoxGoClient"),
		log:     &nop.Logger{},
		retries: defaultRetries,
		env:     env,
	}
}

func (c *HTTPClient) HasTVM() bool {
	return c.tvmc != nil
}

func (c *HTTPClient) R(ctx context.Context) *resty.Request {
	return c.httpc.R().SetContext(ctx)
}

func (c *HTTPClient) Execute(method string, req *resty.Request, result bbtypes.Response) error {
	if c.tvmc != nil {
		var err error
		req, err = c.addTVMTicket(req)
		if err != nil {
			return fmt.Errorf("blackbox: failed to get TVM ticket: %w", err)
		}
	}

	op := func() error {
		rsp, err := req.Execute(method, "/blackbox")
		if err != nil {
			return err
		}

		if rsp.StatusCode() != http.StatusOK {
			return bbtypes.HTTPStatusError{
				Body: rsp.Body(),
				Code: rsp.StatusCode(),
			}
		}

		if err = json.Unmarshal(rsp.Body(), result); err != nil {
			return bbtypes.ParsingError{
				Body: rsp.Body(),
				Err:  err,
			}
		}

		if err := result.CheckError(); err != nil {
			if err.Retryable() {
				return err
			}
			return backoff.Permanent(err)
		}

		return nil
	}

	notify := func(err error, delay time.Duration) {
		c.log.Warn(
			"blackbox: new retry",
			log.Error(err),
			log.Duration("sleep", delay),
			log.String("host", c.env.BlackboxHost),
			log.String("method", req.QueryParam.Get("method")),
			log.String("params", req.QueryParam.Encode()),
		)
	}

	backoffPolicy := c.newBackoff(req.Context())
	return backoff.RetryNotify(op, backoffPolicy, notify)
}

func (c *HTTPClient) Get(req *resty.Request, result bbtypes.Response) error {
	return c.Execute(resty.MethodGet, req, result)
}

func (c *HTTPClient) Post(req *resty.Request, result bbtypes.Response) error {
	return c.Execute(resty.MethodPost, req, result)
}

func (c *HTTPClient) setupTLS() {
	certPool, err := certifi.NewCertPool()
	if err != nil {
		c.log.Error("blackbox: failed to configure TLS cert pool", log.Error(err))
		return
	}

	c.httpc.SetTLSClientConfig(&tls.Config{RootCAs: certPool})
}

func (c *HTTPClient) setTimeout(timeout time.Duration) {
	c.httpc.SetTimeout(timeout)
}

func (c *HTTPClient) setDebug(debug bool) {
	c.httpc.SetDebug(debug)
}

func (c *HTTPClient) setLogger(l log.Structured) {
	c.log = l
	c.httpc.SetLogger(l.Fmt())
}

func (c *HTTPClient) addTVMTicket(req *resty.Request) (*resty.Request, error) {
	tvmTicket, err := c.tvmc.GetServiceTicketForID(req.Context(), c.env.TvmID)
	if err != nil {
		return nil, err
	}

	return req.SetHeader("X-Ya-Service-Ticket", tvmTicket), nil
}

func (c *HTTPClient) newBackoff(ctx context.Context) backoff.BackOff {
	if c.retries <= 1 {
		return &backoff.StopBackOff{}
	}

	exp := backoff.NewExponentialBackOff()
	exp.InitialInterval = backoffInitInternal
	exp.MaxInterval = backoffMaxInterval
	exp.MaxElapsedTime = backoffMaxElapsedTime
	// Reset() must have been called after re-setting fields of ExponentialBackoff.
	// https://github.com/cenkalti/backoff/issues/69#issuecomment-448923118
	exp.Reset()

	// we doing "c.retries - 1", because of "backoff.WithMaxRetries(backoff, 2)" mean repeat operation 3 times.
	return backoff.WithContext(backoff.WithMaxRetries(exp, c.retries-1), ctx)
}
