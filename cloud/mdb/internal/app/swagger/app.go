package swagger

import (
	"net/http"

	openapierrors "github.com/go-openapi/errors"
	"github.com/go-openapi/runtime/middleware"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/httputil/status"
)

func init() {
	flags.RegisterGenConfigFlagGlobal()
	flags.RegisterLogLevelFlagGlobal()
	flags.RegisterConfigPathFlagGlobal()
}

type SwaggerConfig struct {
	Logging httputil.LoggingConfig `json:"logging" yaml:"logging"`
}

func DefaultSwaggerConfig() SwaggerConfig {
	return SwaggerConfig{
		Logging: httputil.DefaultLoggingConfig(),
	}
}

type App struct {
	*app.App

	mdwCtx *middleware.Context
}

// New constructs application suitable for go-swagger services
func New(mdwCtx *middleware.Context, opts ...app.AppOption) (*App, error) {
	a, err := app.New(opts...)
	if err != nil {
		return nil, err
	}

	return &App{App: a, mdwCtx: mdwCtx}, nil
}

// ServeError logs go-swagger serve errors
func (a *App) ServeError(rw http.ResponseWriter, r *http.Request, err error) {
	var oErr openapierrors.Error
	if xerrors.As(err, &oErr) {
		switch oErr.Code() {
		case http.StatusNotFound:
			// Print it with DEBUG, cause we have a lot of NOT_FOUND from scanners
			a.L().Debug("not found error", log.Error(err))
		case http.StatusInternalServerError:
			a.L().Error("internal server error", log.Error(err))
			httputil.ReportErrorToSentry(err, r)
		default:
			if status.GetCodeGroup(int(oErr.Code())) == status.ServerError {
				a.L().Error("50x serve error", log.Error(err), log.Int32("code", oErr.Code()))
			} else {
				a.L().Warn("serve error", log.Error(err), log.Int32("code", oErr.Code()))
			}
		}
	} else {
		a.L().Error("unknown serve error", log.Error(err))
		httputil.ReportErrorToSentry(err, r)
	}
	openapierrors.ServeError(rw, r, err)
}

// LogCallback implements logging for go-swagger server
func (a *App) LogCallback(str string, args ...interface{}) {
	a.L().Infof(str, args...)
}

// MiddlewareContext returns go-swagger's middleware context (used for tracing middleware)
func (a *App) MiddlewareContext() *middleware.Context {
	return a.mdwCtx
}
