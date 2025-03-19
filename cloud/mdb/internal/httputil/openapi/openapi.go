package openapi

import (
	"net/http"
	"time"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/runtime/client"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type RuntimeOption func(*Runtime)

func WithConsumer(contentType string, consumer runtime.Consumer) RuntimeOption {
	return func(r *Runtime) {
		r.r.Consumers[contentType] = consumer
	}
}

// NewRuntime constructs openapi runtime transport
func NewRuntime(
	host string,
	basePath string,
	schemes []string,
	transport http.RoundTripper,
	l log.Logger,
	opts ...RuntimeOption,
) *Runtime {
	c := &http.Client{Transport: transport}
	openapiRuntime := client.NewWithClient(host, basePath, schemes, c)
	r := &Runtime{r: openapiRuntime, l: l}
	for _, opt := range opts {
		opt(r)
	}
	return r
}

// Runtime wraps go-swagger's runtime so we can add logging and error handling to Submit func
type Runtime struct {
	r *client.Runtime
	l log.Logger
}

var _ runtime.ClientTransport = &Runtime{}

func (r *Runtime) Submit(op *runtime.ClientOperation) (interface{}, error) {
	ctx, lfds := httputil.LogEgressStart(op.Context, op.Method, op.PathPattern, r.l)

	op.Context = ctx
	ts := time.Now()
	resp, err := r.r.Submit(op)
	d := time.Since(ts)

	if err != nil {
		err = semerr.WrapWellKnown(err)
	}

	httputil.LogEgressFinish(ctx, lfds, d, err, r.l)
	return resp, err
}

// Sentinels hold sentinel errors to be used as translation targets for go-swagger request errors.
type Sentinels struct {
	NotFound      *xerrors.Sentinel
	BadRequest    *xerrors.Sentinel
	InternalError *xerrors.Sentinel
}

// TranslateError to either semantic error or one of the provided sentinels. User must call this function
// in client code for every error returned by go-swagger.
func TranslateError(err error, sentinels Sentinels) error {
	if err, ok := semerr.WrapWellKnownChecked(err); ok {
		return err
	}

	var e defaultOpenAPIError
	if !xerrors.As(err, &e) {
		return handleCommonErrors(err, sentinels)
	}

	switch {
	case e.Code() == 404:
		if sentinels.NotFound != nil {
			return sentinels.NotFound.Wrap(err)
		}
	case e.Code() >= 400 && e.Code() <= 499:
		if sentinels.BadRequest != nil {
			return sentinels.BadRequest.Wrap(err)
		}
	case e.Code() == 503:
		return semerr.WrapWithUnavailable(err, "unavailable")
	case e.Code() >= 500 && e.Code() <= 599:
		if sentinels.InternalError != nil {
			return sentinels.InternalError.Wrap(err)
		}
	}

	return handleCommonErrors(err, sentinels)
}

func handleCommonErrors(err error, sentinels Sentinels) error {
	var rtErr *runtime.APIError
	if xerrors.As(err, &rtErr) {
		return sentinels.BadRequest.Wrap(err)
	}

	return err
}

// defaultOpenAPIError defines interfaces for when go-swagger spec has default response
type defaultOpenAPIError interface {
	Code() int
}
