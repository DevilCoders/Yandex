package httputil

import (
	"context"
	"net/http"
	"net/url"
	"path"
	"time"

	"github.com/opentracing-contrib/go-stdlib/nethttp"
	"github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/ext"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/httputil/status"
)

// Client wraps http.Client overriding Do function and providing
// request logging and tracing
type Client struct {
	*http.Client
	l   log.Logger
	cfg ClientConfig
}

// ClientConfig for HTTP clients
type ClientConfig struct {
	Name      string          `json:"name" yaml:"name"`
	Transport TransportConfig `json:"transport" yaml:"transport"`
}

// NewClient constructs Client
func NewClient(cfg ClientConfig, l log.Logger) (*Client, error) {
	rt, err := NewTransport(cfg.Transport, l)
	if err != nil {
		return nil, err
	}

	return &Client{Client: &http.Client{Transport: rt}, l: l}, nil
}

// DEPRECATEDNewClient constructs Client
func DEPRECATEDNewClient(c *http.Client, name string, l log.Logger) *Client {
	return &Client{Client: c, cfg: ClientConfig{Name: name}, l: l}
}

func (c Client) Do(req *http.Request, opName string, tags ...opentracing.Tag) (*http.Response, error) {
	ctx, lfds := LogEgressStart(req.Context(), req.Method, req.URL.String(), c.l)
	resp, d, err := c.do(req.WithContext(ctx), opName, tags...)
	LogEgressFinish(ctx, lfds, d, err, c.l)
	return resp, err
}

func (c Client) do(req *http.Request, opName string, tags ...opentracing.Tag) (*http.Response, time.Duration, error) {
	req, ht := nethttp.TraceRequest(
		opentracing.GlobalTracer(),
		req,
		nethttp.ComponentName(c.cfg.Name),
		nethttp.OperationName(opName),
		nethttp.ClientSpanObserver(func(span opentracing.Span, r *http.Request) {
			for _, tag := range tags {
				span.SetTag(tag.Key, tag.Value)
			}
		}),
	)
	defer ht.Finish()

	ts := time.Now()
	resp, err := c.Client.Do(req)
	if err != nil {
		err = semerr.WrapWellKnown(err)
		tracing.SetErrorOnSpan(ht.Span(), err)
	} else {
		switch status.GetCodeGroup(resp.StatusCode) {
		case status.ClientError, status.ServerError:
			ext.Error.Set(ht.Span(), true)
		}
	}

	return resp, time.Since(ts), err
}

func LogEgressStart(ctx context.Context, method, url string, l log.Logger) (context.Context, []log.Field) {
	lfds := []log.Field{
		log.String("egress", "HTTP"),
		log.String("egress_http_method", method),
		log.String("egress_url", url),
	}
	ctx = ctxlog.WithFields(ctx, lfds...)
	ctxlog.Debug(ctx, l, "client request started")
	return ctx, lfds
}

func LogEgressFinish(ctx context.Context, lfds []log.Field, d time.Duration, err error, l log.Logger) {
	lfds = lfds[:0]
	lfds = append(lfds, log.Duration("duration", d))
	if err != nil {
		lfds = append(lfds, log.Error(err))
	}

	ctxlog.Debug(ctx, l, "client request finished", lfds...)
}

func JoinURL(host string, requestPath string) (string, error) {
	u, err := url.Parse(host)
	if err != nil {
		return "", err
	}

	u.Path = path.Join(u.Path, requestPath)
	return u.String(), nil
}
